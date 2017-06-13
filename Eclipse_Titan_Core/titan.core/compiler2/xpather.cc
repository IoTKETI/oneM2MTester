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
 *   Delic, Adam
 *   Kovacs, Ferenc
 *   Ormandi, Matyas
 *   Pandi, Krisztian
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#include "xpather.h"

#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#define LIBXML_SCHEMAS_ENABLED
#include <libxml/xmlschemastypes.h>

#include "../common/memory.h"
#include "vector.hh"
// Do _NOT_ #include "string.hh", it drags in ustring.o, common/Quadruple.o,
// Int.o, ttcn3/PatternString.o, and then the entire AST :(
#include "map.hh"
#include "ProjectGenHelper.hh"
#include "../common/path.h"
#include "ttcn3/ttcn3_preparser.h"
#include "asn1/asn1_preparser.h"

// in makefile.c
void ERROR  (const char *fmt, ...);
void WARNING(const char *fmt, ...);
void NOTIFY (const char *fmt, ...);
void DEBUG  (const char *fmt, ...);

// for vector and map
void fatal_error(const char * filename, int lineno, const char * fmt, ...)
__attribute__ ((__format__ (__printf__, 3, 4), __noreturn__));

void fatal_error(const char * filename, int lineno, const char * fmt, ...)
{
  fputs(filename, stderr);
  fprintf(stderr, ":%d: ", lineno);
  va_list va;
  va_start(va, fmt);
  vfprintf(stderr, fmt, va);
  va_end(va);
  abort();
}

static ProjectGenHelper& projGenHelper = ProjectGenHelper::Instance();

/// Run an XPath query and return an xmlXPathObjectPtr, which must be freed
xmlXPathObjectPtr run_xpath(xmlXPathContextPtr xpathCtx, const char *xpathExpr)
{
  xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(
    (const xmlChar *)xpathExpr, xpathCtx);
  if(xpathObj == NULL) {
    fprintf(stderr,"Error: unable to evaluate xpath expression \"%s\"\n", xpathExpr);
    return 0;
  }

  return xpathObj;
}

// RAII classes

class XmlDoc {
public:
  explicit XmlDoc(xmlDocPtr p) : doc_(p) {}
  ~XmlDoc() {
    if (doc_ != NULL) xmlFreeDoc(doc_);
  }
  operator xmlDocPtr() const { return doc_; }
private:
  xmlDocPtr doc_;
};

class XPathContext {
public:
  explicit XPathContext(xmlXPathContextPtr c) : ctx_(c) {}
  ~XPathContext() {
    if (ctx_ != NULL) xmlXPathFreeContext(ctx_);
  }
  operator xmlXPathContextPtr() const { return ctx_; }
private:
  xmlXPathContextPtr ctx_;
};

class XPathObject {
public:
  explicit XPathObject(xmlXPathObjectPtr o) : xpo_(o) {}
  ~XPathObject() {
    if (xpo_ != NULL) xmlXPathFreeObject(xpo_);
  }
  operator   xmlXPathObjectPtr() const { return xpo_; }
  xmlXPathObjectPtr operator->() const { return xpo_; }
private:
  xmlXPathObjectPtr xpo_;
};

//------------------------------------------------------------------
/// compare-by-content wrapper of a plain C string
struct cstring {
  explicit cstring(const char *s) : str(s) {}
  void destroy() const;
  operator const char*() const { return str; }
protected:
  const char *str;
  friend boolean operator<(const cstring& l, const cstring& r);
  friend boolean operator==(const cstring& l, const cstring& r);
};

void cstring::destroy() const {
  Free(const_cast<char*>(str)); // assumes valid pointer or NULL
}

boolean operator<(const cstring& l, const cstring& r) {
  return strcmp(l.str, r.str) < 0;
}

boolean operator==(const cstring& l, const cstring& r) {
  return strcmp(l.str, r.str) == 0;
}

/// RAII for C string
struct autostring : public cstring {
  /// Constructor; takes over ownership
  explicit autostring(const char *s = 0) : cstring(s) {}
  ~autostring() {
    // He who can destroy a thing, controls that thing -- Paul Muad'Dib
    Free(const_cast<char*>(str)); // assumes valid pointer or NULL
  }
  /// %Assignment; takes over ownership
  const autostring& operator=(const char *s) {
    Free(const_cast<char*>(str)); // assumes valid pointer or NULL
    str = s;
    return *this;
  }
  /// Relinquish ownership
  const char *extract() {
    const char *retval = str;
    str = 0;
    return retval;
  }
private:
  autostring(const autostring&);
  autostring& operator=(const autostring&);
};


bool validate_tpd(const XmlDoc& xml_doc, const char* tpd_file_name, const char* xsd_file_name)
{
  xmlLineNumbersDefault(1);

  xmlSchemaParserCtxtPtr ctxt = xmlSchemaNewParserCtxt(xsd_file_name);
  if (ctxt==NULL) {
    ERROR("Unable to create xsd context for xsd file `%s'", xsd_file_name);
    return false;
  }
  xmlSchemaSetParserErrors(ctxt, (xmlSchemaValidityErrorFunc)fprintf, (xmlSchemaValidityWarningFunc)fprintf, stderr);

  xmlSchemaPtr schema = xmlSchemaParse(ctxt);
  if (schema==NULL) {
    ERROR("Unable to parse xsd file `%s'", xsd_file_name);
    xmlSchemaFreeParserCtxt(ctxt);
    return false;
  }

  xmlSchemaValidCtxtPtr xsd = xmlSchemaNewValidCtxt(schema);
  if (xsd==NULL) {
    ERROR("Schema validation error for xsd file `%s'", xsd_file_name);
    xmlSchemaFree(schema);
    xmlSchemaFreeParserCtxt(ctxt);
    return false;
  }
  xmlSchemaSetValidErrors(xsd, (xmlSchemaValidityErrorFunc) fprintf, (xmlSchemaValidityWarningFunc) fprintf, stderr);

  int ret = xmlSchemaValidateDoc(xsd, xml_doc);

  xmlSchemaFreeValidCtxt(xsd);
  xmlSchemaFree(schema);
  xmlSchemaFreeParserCtxt(ctxt);
  xmlSchemaCleanupTypes();

  if (ret==0) {
    return true; // successful validation
  } else if (ret>0) {
    ERROR("TPD file `%s' is invalid according to schema `%s'", tpd_file_name, xsd_file_name);
    return false;
  } else {
    ERROR("TPD validation of `%s' generated an internal error in libxml2", tpd_file_name);
    return false;
  }
}

/** Extract a boolean value from the XML, if it exists otherwise flag is unchanged
 *
 * @param xpathCtx XPath context object
 * @param actcfg name of the active configuration
 * @param option name of the value
 * @param flag pointer to the variable to receive the value
 */
void xsdbool2boolean(const XPathContext& xpathCtx, const char *actcfg,
  const char *option, boolean* flag)
{
  char *xpath = mprintf(
    "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
    "/ProjectProperties/MakefileSettings/%s[text()='true']",
    actcfg, option);
  XPathObject xpathObj(run_xpath(xpathCtx, xpath));
  Free(xpath);

  if (xpathObj->nodesetval && xpathObj->nodesetval->nodeNr > 0) {
    *flag = TRUE;
  }
}

extern "C" string_list* getExternalLibs(const char* projName)
{
  if (!projGenHelper.getZflag()) return NULL;
  ProjectDescriptor* proj = projGenHelper.getTargetOfProject(projName);
  if (!proj) return NULL;

  std::vector<const char*> externalLibs;
  projGenHelper.getExternalLibs(externalLibs);

  if (0 == externalLibs.size()) return NULL;

  struct string_list* head = (struct string_list*)Malloc(sizeof(struct string_list));
  struct string_list* last_elem = head;
  struct string_list* tail = head;

  for (size_t i = 0; i < externalLibs.size(); ++i) {
     tail = last_elem;
     last_elem->str = mcopystr(externalLibs[i]);
     last_elem->next = (struct string_list*)Malloc(sizeof(struct string_list));
     last_elem = last_elem->next;
  }
  Free(last_elem);
  tail->next = NULL;
  return head;
}

extern "C" string_list* getExternalLibPaths(const char* projName)
{
  if (!projGenHelper.getZflag()) return NULL;
  ProjectDescriptor* proj = projGenHelper.getTargetOfProject(projName);
  if (!proj) return NULL;

  std::vector<const char*> externalLibs;
  projGenHelper.getExternalLibSearchPaths(externalLibs);

  if (0 == externalLibs.size()) return NULL;

  struct string_list* head = (struct string_list*)Malloc(sizeof(struct string_list));
  struct string_list* last_elem = head;
  struct string_list* tail = head;

  for (size_t i = 0; i < externalLibs.size(); ++i) {
     tail = last_elem;
     last_elem->str = mcopystr(externalLibs[i]);
     last_elem->next = (struct string_list*)Malloc(sizeof(struct string_list));
     last_elem = last_elem->next;
  }
  Free(last_elem);
  tail->next = NULL;
  return head;
}

extern "C" string_list* getRefWorkingDirs(const char* projName)
{
  if (!projGenHelper.getZflag()) return NULL;
  ProjectDescriptor* proj = projGenHelper.getTargetOfProject(projName);
  if (!proj) FATAL_ERROR("Project \"%s\" was not found in the project list", projName);

  struct string_list* head = (struct string_list*)Malloc(sizeof(struct string_list));
  struct string_list* last_elem = head;
  struct string_list* tail = head;
  last_elem->str = NULL;
  last_elem->next = NULL;
  for (size_t i = 0; i < proj->numOfRefProjWorkingDirs(); ++i) {
    tail = last_elem;
    last_elem->str = mcopystr(proj->getRefProjWorkingDir(i).c_str());
    last_elem->next = (struct string_list*)Malloc(sizeof(struct string_list));
    last_elem = last_elem->next;
  }
  Free(last_elem);
  tail->next = NULL;
  return head;
}

extern "C" string2_list* getLinkerLibs(const char* projName)
{

  if (!projGenHelper.getZflag()) return NULL;
  if (1 == projGenHelper.numOfProjects() || 0 == projGenHelper.numOfLibs()){
    return NULL; //no library
  }
  ProjectDescriptor* projLib = projGenHelper.getTargetOfProject(projName);
  if (!projLib) FATAL_ERROR("Project \"%s\" was not found in the project list", projName);

  struct string2_list* head = (struct string2_list*)Malloc(sizeof(struct string2_list));
  struct string2_list* last_elem = head;
  struct string2_list* tail = head;
  last_elem->next = NULL;
  last_elem->str1 = NULL;
  last_elem->str2 = NULL;
  for (std::map<std::string, ProjectDescriptor>::const_iterator it = projGenHelper.getHead();
       it != projGenHelper.getEnd(); ++it) {
    if ((it->second).isLibrary()) {
      if (!(it->second).getLinkingStrategy() &&
          !projLib->hasLinkerLibTo((it->second).getProjectName())) { // static linked library
          continue;
      }
      std::string relPath = projLib->setRelativePathTo((it->second).getProjectAbsWorkingDir());
      if (relPath == std::string(".")) {
        continue; // the relpath shows to itself
      }
      tail = last_elem;
      last_elem->str1 = mcopystr(relPath.c_str());
      last_elem->str2 = mcopystr((it->second).getTargetExecName().c_str());
      last_elem->next = (struct string2_list*)Malloc(sizeof(struct string2_list));
      last_elem = last_elem->next;
    }
  }
  tail->next = NULL;
  Free(last_elem);

  if (head->str1 && head->str2)
    return head;
  else
    return NULL;
}

extern "C" const char* getLibFromProject(const char* projName)
{
  if (!projGenHelper.getZflag()) return NULL;
  ProjectDescriptor* lib = projGenHelper.getTargetOfProject(projName);
  if (lib) return lib->getTargetExecName().c_str();
  return NULL;
}

extern "C" void erase_libs(void) {
  projGenHelper.cleanUp();
}

extern "C" void print_libs(void) {
  projGenHelper.print();
}


extern "C" boolean hasSubProject(const char* projName) {
  if (!projGenHelper.getZflag()) return FALSE;
  if (projGenHelper.getHflag())
    return static_cast<boolean>(projGenHelper.hasReferencedProject());
  else if(std::string(projName) == projGenHelper.getToplevelProjectName())
    return static_cast<boolean>(projGenHelper.hasReferencedProject());
  else
    return FALSE;
}

extern "C" boolean hasExternalLibrary(const char* libName, const char* projName) {
  if (!projGenHelper.getZflag()) return FALSE;
  ProjectDescriptor* projLib = projGenHelper.getTargetOfProject(projName);
  if (projLib && projLib->hasLinkerLib(libName))
    return TRUE;
  else
    return FALSE;
}

extern "C" boolean isTopLevelExecutable(const char* projName) {
  if (!projGenHelper.getZflag()) return false;
  ProjectDescriptor* proj = projGenHelper.getTargetOfProject(projName);
  if (projGenHelper.getToplevelProjectName() != std::string(projName)) return FALSE;
  if (proj && proj->isLibrary())
    return FALSE;
  else
    return TRUE;
}

extern "C" boolean isDynamicLibrary(const char* key) {
  if (!projGenHelper.getZflag()) return false;
  ProjectDescriptor* proj = projGenHelper.getProjectDescriptor(key);
  if (proj) return proj->getLinkingStrategy();
  FATAL_ERROR("Library \"%s\" was not found", key);
  return false;
}

extern "C" const char* getTPDFileName(const char* projName) {
  if (!projGenHelper.getZflag()) return NULL;
  ProjectDescriptor* proj = projGenHelper.getTargetOfProject(projName);
  if (proj) return proj->getTPDFileName().c_str();
  FATAL_ERROR("TPD file name to project \"%s\" was not found", projName);
}

extern "C" const char* getPathToRootDir(const char* projName) {
  if (!projGenHelper.getZflag()) return NULL;
  ProjectDescriptor* proj = projGenHelper.getTargetOfProject(projName);
  const char* rootDir = projGenHelper.getRootDirOS(projName).c_str();
  if (proj && rootDir) {
    return rootDir;
  }
  FATAL_ERROR("Project \"%s\": no relative path was found to top directory at OS level.", projName);
}

extern "C" const char* findLibraryPath(const char* libraryName, const char* projName)
{
  if (!projGenHelper.getZflag()) return NULL;
  ProjectDescriptor* projLib = projGenHelper.getTargetOfProject(projName);
  if (!projLib) FATAL_ERROR("Project \"%s\" was not found in the project list", projName);
  ProjectDescriptor* libLib = projGenHelper.getProjectDescriptor(libraryName);
  if (!libLib) return NULL;
  std::string str = projLib->setRelativePathTo(libLib->getProjectAbsWorkingDir());
  size_t refIndex = projLib->getLibSearchPathIndex(libLib->getProjectName());
  if (refIndex > projLib->numOfLibSearchPaths()) return NULL;
  projLib->setLibSearchPath(refIndex, str);
  return projLib->getLibSearchPath(libLib->getProjectName());
}

extern "C" const char* findLibraryName(const char* libraryName, const char* projName)
{
  if (!projGenHelper.getZflag()) return NULL;
  ProjectDescriptor* projLib = projGenHelper.getTargetOfProject(projName);
  if (!projLib) FATAL_ERROR("Project \"%s\" was not found in the project list", projName);
  ProjectDescriptor* libLib = projGenHelper.getProjectDescriptor(libraryName);
  if (!libLib) return NULL;
  for (size_t i = 0; i < projLib->numOfReferencedProjects(); ++i) {
    const std:: string refProjName = projLib->getReferencedProject(i);
    ProjectDescriptor* refLib = projGenHelper.getTargetOfProject(refProjName.c_str());
    if (refLib->getTargetExecName() == std::string(libraryName))
      return libraryName;
  }
  return NULL;
}

extern "C" boolean isTtcn3ModuleInLibrary(const char* moduleName)
{
  if (!projGenHelper.getZflag()) return FALSE;
  return (boolean)projGenHelper.isTtcn3ModuleInLibrary(moduleName);
}

extern "C" boolean isAsn1ModuleInLibrary(const char* moduleName)
{
  if (!projGenHelper.getZflag()) return FALSE;
  return (boolean)projGenHelper.isAsn1ModuleInLibrary(moduleName);
}

extern "C" boolean isSourceFileInLibrary(const char* fileName)
{
  if (!projGenHelper.getZflag()) return FALSE;
  return (boolean)projGenHelper.isSourceFileInLibrary(fileName);
}

extern "C" boolean isHeaderFileInLibrary(const char* fileName)
{
  if (!projGenHelper.getZflag()) return FALSE;
  return (boolean)projGenHelper.isHeaderFileInLibrary(fileName);
}

extern "C" boolean isTtcnPPFileInLibrary(const char* fileName)
{
  if (!projGenHelper.getZflag()) return FALSE;
  return (boolean)projGenHelper.isTtcnPPFileInLibrary(fileName);
}

extern "C" boolean isXSDModuleInLibrary(const char* fileName)
{
  if (!projGenHelper.getZflag()) return FALSE;
  return (boolean)projGenHelper.isXSDModuleInLibrary(fileName);
}

extern "C" boolean buildObjects(const char* projName, boolean add_referenced)
{
  if (!projGenHelper.getZflag()) return FALSE;
  if (projGenHelper.getHflag()) return FALSE;
  if (add_referenced) return FALSE;
  ProjectDescriptor* desc =projGenHelper.getTargetOfProject(projName);
  if (desc && desc->isLibrary()) return FALSE;
  return TRUE;
}

void append_to_library_list (const char* prjName,
                             const XPathContext& xpathCtx,
                             const char *actcfg)
{
  if (!projGenHelper.getZflag()) return;

  char *exeXpath = mprintf(
    "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
    "/ProjectProperties/MakefileSettings/targetExecutable/text()",
    actcfg);
  XPathObject exeObj(run_xpath(xpathCtx, exeXpath));
  Free(exeXpath);
  std::string lib_name;
  if (exeObj->nodesetval && exeObj->nodesetval->nodeNr > 0) {
    const char* target_executable = (const char*)exeObj->nodesetval->nodeTab[0]->content;
    autostring target_exe_dir(get_dir_from_path(target_executable));
    autostring target_exe_file(get_file_from_path(target_executable));
    lib_name = target_exe_file;
    ProjectDescriptor* projDesc = projGenHelper.getTargetOfProject(prjName);
    if (projDesc) {
      projDesc->setTargetExecName(lib_name.c_str());
    }
  }
}

// data structures and functions to manage excluded folders/files

static map<cstring, const char> excluded_files;

boolean is_excluded_file(const cstring& path, const char* project) {
  if (!excluded_files.has_key(path)) return false;
  const char* proj = excluded_files[path];
  if (0 == strcmp(project, proj)) return true;
  return false;
}

static vector<const char> excluded_folders;

// Unfortunately, when "docs" is excluded, we need to drop
// files in "docs/", "docs/pdf/", "docs/txt/", "docs/txt/old/" etc;
// so it's not as simple as using a map :(

/** Checks whether a file is under an excluded folder
 *
 * @param path (relative) path of the file
 * @return true if file is excluded, false otherwise
 */
boolean is_excluded_folder(const char *path) {
  boolean answer = FALSE;
  size_t pathlen = strlen(path);

  for (size_t i = 0, end = excluded_folders.size(); i < end; ++i) {
    const char *xdir = excluded_folders[i];
    size_t xdlen = strlen(xdir);
    if (pathlen > xdlen && path[xdlen] == '/') {
      // we may have a winner
      if ((answer = !strncmp(path, xdir, xdlen))) break;
    }
  }
  return answer;
}

// How do you treat a raw info? You cook it, of course!
// Returns a newly allocated string.
char *cook(const char *raw, const map<cstring, const char>& path_vars)
{
  const char *slash = strchr(raw, '/');
  if (!slash) { // Pretend that the slash is at the end of the string.
    slash = raw + strlen(raw);
  }

  // Assume that a path variable reference is the first (or only) component
  // of the path: ROOT in "ROOT/etc/issue".
  autostring prefix(mcopystrn(raw, slash - raw));
  if (path_vars.has_key(prefix)) {
    char *cooked = mcopystr(path_vars[prefix]);
    bool ends_with_slash = cooked[strlen(cooked)-1] == '/';
    if (ends_with_slash && *slash == '/') {
      // Avoid paths with two slashes at the start; Cygwin thinks it's UNC
      ++slash;
    }
    // If there was no '/' (only the path variable reference e.g "ROOT")
    // then slash points to a null byte and the mputstr is a no-op.
    cooked = mputstr(cooked, slash);
    return cooked;
  }

  // If the path variable could not be substituted,
  // return (a copy of) the original.
  return mcopystr(raw);
}

void replacechar(char** content) {

  std::string s= *content;
  size_t found = 0;
  while ((found = s.find('['))!= std::string::npos){
    s.replace(found,1, "${");
  }
  while ((found = s.find(']')) != std::string::npos){
    s.replace(found,1, "}");
  }
  *content = mcopystr(s.c_str());
}

/** Determines the suffix (i.e. the character sequence following the last dot)
 * of file or path name \a file_name. NULL pointer is returned if \a file_name
 * does not contain any dot character or the last character of it is a dot.
 * The suffix is not copied, the returned pointer points to the tail of
 * \a file_name. */
const char *get_suffix(const char *file_name)
{
  size_t last_dot = (size_t)-1;
  size_t i;
  for (i = 0; file_name[i] != '\0'; i++)
    if (file_name[i] == '.') last_dot = i;
  if (last_dot == (size_t)-1 || file_name[last_dot + 1] == '\0') return NULL;
  else return file_name + last_dot + 1;
}

int is_xsd_module(const char *file_name, char **module_name) {
  const char * extension = get_suffix(file_name);
  if (extension == NULL) {
    return 0;
  }
  if (strcmp(extension, "xsd") != 0) {
    return 0;
  }
  if (module_name != NULL) *module_name = NULL;
  FILE *fp;
  char line[1024];
  char *command = NULL;
  char *ttcn3_dir = getenv("TTCN3_DIR");
  command = mputprintf(command, "%s%sxsd2ttcn -q -n %s",
    ttcn3_dir != NULL ? ttcn3_dir : "",
    ttcn3_dir != NULL ? "/bin/" : "",
    file_name);
  fp = popen(command, "r");
  Free(command);
  if (fp == NULL) {
    ERROR("Could not get the module names of the XSD modules");
    return 0;
  }
  while (fgets(line, sizeof(line)-1, fp) != NULL) {
    *module_name = mputstr(*module_name, line);
  }
  int rv= pclose(fp);
  if (rv > 0) {
    return 0;
  }
  return *module_name != NULL;
}

static void clear_seen_tpd_files(map<cstring, int>& seen_tpd_files) {
  for (size_t i = 0, num = seen_tpd_files.size(); i < num; ++i) {
    const cstring& key = seen_tpd_files.get_nth_key(i);
    int *elem = seen_tpd_files.get_nth_elem(i);
    key.destroy();
    delete elem;
  }
  seen_tpd_files.clear();
}

const char* get_act_config(struct string2_list* cfg, const char* project_name) {
  while (cfg && cfg->str1 && project_name) {
    if (!strcmp(cfg->str1, project_name)) return cfg->str2;
    cfg = cfg->next;
  }
  return NULL;
}

void free_string_list(struct string_list* act_elem)
{
  while (act_elem) {
    struct string_list* next_elem = act_elem->next;
    Free(act_elem->str);
    Free(act_elem);
    act_elem = next_elem;
  }
}

void free_string2_list(struct string2_list* act_elem)
{
  while (act_elem) {
    struct string2_list* next_elem = act_elem->next;
    Free(act_elem->str1);
    Free(act_elem->str2);
    Free(act_elem);
    act_elem = next_elem;
  }
}

void free_config_list(struct config_list* act_elem) {
  while (act_elem) {
    struct config_list* next_elem = act_elem->next;
    Free(act_elem->str1);
    Free(act_elem->str2);
    Free(act_elem);
    act_elem = next_elem;
  }
}

void free_config_struct(struct config_struct* act_elem) {
  while (act_elem) {
    struct config_struct* next_elem = act_elem->next;
    Free(act_elem->project_name);
    Free(act_elem->project_conf);
    free_string_list(act_elem->dependencies);
    free_string2_list(act_elem->requirements);
    free_string_list(act_elem->children);
    Free(act_elem);
    act_elem = next_elem;
  }
}

// Initialize a config_struct to NULL-s
static void config_struct_init(struct config_struct* const list) {
  list->project_name = list->project_conf = NULL;
  list->is_active = FALSE;
  list->dependencies = (struct string_list*)Malloc(sizeof(struct string_list));
  list->dependencies->str = NULL;
  list->dependencies->next = NULL;
  list->requirements = (struct string2_list*)Malloc(sizeof(struct string2_list));
  list->requirements->str1 = NULL;
  list->requirements->str2 = NULL;
  list->requirements->next = NULL;
  list->children = (struct string_list*)Malloc(sizeof(struct string_list));
  list->children->str = NULL;
  list->children->next = NULL;
  list->processed = FALSE;
  list->next = NULL;
}

// This function fills up the dependencies field of the config_structs
// using the children fields.
static void config_struct_fillup_deps(struct config_struct* const list) {
  struct config_struct* last = list;
  while (last && last->project_name != NULL) {  // Go through the projects
    struct config_struct* lastest = list;
    while (lastest && lastest->project_name != NULL) { // Go through the projects again n^2 complexity
      struct string_list* lastest_child = lastest->children;
      while (lastest_child && lastest_child->str != NULL) {
        if (strcmp(last->project_name, lastest_child->str) == 0) { // If a project is a child of another project
          // Add the other project to the project's dependencies
          // But first check if it is already in the dependencies
          boolean already_in = FALSE;
          struct string_list* last_dep = last->dependencies;
          while (last_dep && last_dep->str != NULL) {
            if (strcmp(last_dep->str, lastest->project_name) == 0) {
              already_in = TRUE;
              break;
            }
            last_dep = last_dep->next;
          }
          if (already_in == FALSE) {
            last_dep->str = mcopystr(lastest->project_name);
            struct string_list* last_dep_next = (struct string_list*)Malloc(sizeof(string_list));
            last_dep_next->str = NULL;
            last_dep_next->next = NULL;
            last_dep->next = last_dep_next;
            break;
          }
        }
        lastest_child = lastest_child->next;
      }
      lastest = lastest->next;
    }
    last = last->next;
  }
}

// This function inserts project_name project's project_config configuration
// into the required configurations. This function can detect errors.
// If an error is detected FALSE is returned, otherwise TRUE.
static boolean insert_to_required_config(const struct config_struct* all_configs, const char* project_name, const char* project_config, struct string2_list* const required_configs) {

  boolean found_project = FALSE;
  boolean found_config = FALSE;
  boolean result = TRUE;
  
  //Check that it is a valid configuration for a valid project
  const struct config_struct* last = all_configs;
  while(last && last->project_name != NULL && last->project_conf != NULL) {
    if (strcmp(last->project_name, project_name) == 0) {
      found_project = TRUE;
      if (strcmp(last->project_conf, project_config) == 0) {
        found_config = TRUE;
        break;
      }
    }
    last = last->next;
  }
  // If the project or the configuration is not found
  if (found_project == FALSE || found_config == FALSE) {
    result = FALSE;
  }
  
  // Check if the project is already in the required configurations
  // or if the project is present with a different configuration
  boolean already_in = FALSE;
  struct string2_list* last_required_config = required_configs;
  while (last_required_config && last_required_config->str1 != NULL && last_required_config->str2 != NULL) {
    // If we have a record of this project
    if (strcmp(last_required_config->str1, project_name) == 0) {
      if (strcmp(last_required_config->str2, project_config) == 0) {
        // This project configuration is already in the required_configs
        already_in = TRUE;
      } else {
        // This project configuration is different than it is in the required_configs
        result = FALSE;
      }
    }
    last_required_config = last_required_config->next;
  }
  // If the project's configuration is not already present in the required
  // configs then we insert it. We insert it even when the result is FALSE.
  if (last_required_config && already_in == FALSE) {
    // Make copies of strings
    last_required_config->str1 = mcopystr(project_name);
    last_required_config->str2 = mcopystr(project_config);
    // Init next required configuration
    struct string2_list* last_required_config_next = (struct string2_list*)Malloc(sizeof(struct string2_list));
    last_required_config_next->str1 = NULL;
    last_required_config_next->str2 = NULL;
    last_required_config_next->next = NULL;
    last_required_config->next = last_required_config_next;
  }
  return result;
}

// Inserts project_name project with project_config configuration to the tmp_configs
// Return TRUE if the configuration is inserted
// Returns FALSE if the configuration is not inserted
// Returns 2 if the configuration is already in the tmp_configs (still inserted)
static boolean insert_to_tmp_config(struct config_list* const tmp_configs, const char* project_name, const char* project_config, const boolean is_active) {
  //First we check that it is a valid configuration for a valid project
  boolean found_project = FALSE;
  boolean found_config = FALSE;
  boolean active = FALSE;
  boolean result = TRUE;
  
  //Check if we have the same project with same configuration in the tmp_configs
  struct config_list* last = tmp_configs;
  while(last && last->str1 != NULL && last->str2 != NULL) {
    if (strcmp(last->str1, project_name) == 0) {
      found_project = TRUE;
      active = last->is_active;
      if (strcmp(last->str2, project_config) == 0) {
        found_config = TRUE;
        break;
      }
    }
    last = last->next;
  }
  
  //The case of same project with same configuration
  if (found_project == TRUE && found_config == TRUE) {
    result = 2;
    
  // The case of same project different configuration and the configuration
  // is not default
  } else if(found_project == TRUE && active == FALSE) {
    return FALSE;
  }
  // Go to the end of list
  while (last->next) {
    last = last->next;
  }
  // Insert new config into the tmp_configs
  last->str1 = mcopystr(project_name); 
  last->str2 = mcopystr(project_config);
  last->is_active = is_active;
  last->next = (struct config_list*)Malloc(sizeof(config_list));
  last->next->str1 = NULL;
  last->next->str2 = NULL;
  last->next->is_active = FALSE;
  last->next->next = NULL;

  return result;
}

// Removes the last element from the tmp_configs
static void remove_from_tmp_config(struct config_list* const tmp_configs, const char* /*project_name*/, const char* /*project_config*/) {
  struct config_list* last = tmp_configs;
  while (last->next && last->next->next != NULL) {
    last = last->next;
  }

  Free(last->str1);
  Free(last->str2);
  last->str1 = NULL;
  last->str2 = NULL;
  last->is_active = FALSE;
  Free(last->next);
  last->next = NULL;
}

// This function detects a circle originating from start_project.
// act_project is the next project which might be in the circle
// needed_in is a project that is needed to be inside the circle
// list is a temporary list which contains the elements of circle
static boolean is_circular_dep(const struct config_struct* all_configs, const char* start_project, const char* act_project, const char* needed_in, struct string_list** list) {
  if (*list == NULL) {
    *list = (struct string_list*)Malloc(sizeof(string_list));
    (*list)->str = mcopystr(start_project);
    (*list)->next = (struct string_list*)Malloc(sizeof(string_list));
    (*list)->next->str = NULL;
    (*list)->next->next = NULL;
  }
  //Circle detection
  struct string_list* last_list = *list;
  while (last_list && last_list->str != NULL) {
    struct string_list* last_list2 = last_list;
    while (last_list2 && last_list2->str != NULL) {
      // If the pointers are different but the project name is the same
      if (last_list != last_list2 && strcmp(last_list->str, last_list2->str) == 0) {
        // We look for the circle which starts from the start_project
        if (strcmp(last_list->str, start_project) != 0) {
          return FALSE;
        } else {
          // Check if needed_in is inside the circle
          while (last_list != last_list2) {
            if (needed_in != NULL && strcmp(last_list->str, needed_in) == 0) {
              return TRUE;
            }
            last_list = last_list->next;
          }
          return FALSE;
        }
      }
      last_list2 = last_list2->next;
    }
    last_list = last_list->next;
  }
  // Insert next element and call recursively for all the referenced projects
  const struct config_struct* last = all_configs;
  while (last && last->project_name) {
    //Find an act_project configuration
    if (strcmp(last->project_name, act_project) == 0) {
      // Go through its children
      struct string_list* children = last->children;
      while (children && children->str != NULL) {
        // Insert child into list
        last_list->str = mcopystr(children->str);
        last_list->next = (struct string_list*)Malloc(sizeof(string_list));
        last_list->next->str = NULL;
        last_list->next->next = NULL;
        // Call recursively
        if (is_circular_dep(all_configs, start_project, children->str, needed_in, list) == TRUE) {
          return TRUE;
        }
        // Remove child
        last_list = *list;
        while(last_list && last_list->next != NULL && last_list->next->next != NULL) {
          last_list = last_list->next;
        }
        Free(last_list->str);
        Free(last_list->next);
        last_list->str = NULL;
        last_list->next = NULL;
        children = children->next;
      }
    }
    last = last->next;
  }
  return FALSE;
}

// check if project_name project does not exists todo
// Project config == NULL means that we need to analyse the children of the project, todo why
// This function analyses project_name project to get the required configuration
// of this project.
// project_config may be NULL. It is not null when we are not certain of the
// project_name project's configuration.
// tmp_configs acts like a stack. It contains the history calls of anal_child. It
// is used to detect circles therefore prevents infinite recursions.
static boolean analyse_child(struct config_struct* const all_configs, const char* project_name, const char* project_config, struct string2_list* const required_configs, struct config_list* const tmp_configs) {
  boolean result = TRUE;
  const char* act_config = get_act_config(required_configs, project_name);
  // If the required configuration is known of project_name project
  if (act_config != NULL) {
    struct config_struct* tmp = all_configs;
    // Get the project_name's act_config config_struct (tmp holds it)
    while (tmp && tmp->project_name != NULL && tmp->project_conf != NULL) {
      if (strcmp(tmp->project_name, project_name) == 0 && strcmp(tmp->project_conf, act_config) == 0) {
        break;
      }
      tmp = tmp->next;
    }
    if (tmp->processed == TRUE) {
      // We already processed this project there is nothing to be done
      return result;
    }
    // Set the project to processed
    tmp->processed = TRUE;
    // Get the last (empty) required configuration
    struct string2_list* last_required_config = required_configs;
    while (last_required_config->next != NULL) {
      last_required_config = last_required_config->next;
    }
    // Insert all required config of project_name project's act_config configuration
    struct string2_list* last_proj_config = tmp->requirements;
    while (last_proj_config && last_proj_config->str1 != NULL && last_proj_config->str2 != NULL) {
      insert_to_required_config(all_configs, last_proj_config->str1, last_proj_config->str2, required_configs);
      last_proj_config = last_proj_config->next;
    }
    // Analyse the children of this project too.
    insert_to_tmp_config(tmp_configs, project_name, act_config, TRUE);
    struct string_list* last_child = tmp->children;
    while (last_child && last_child->str != NULL) {
      result = analyse_child(all_configs, last_child->str, NULL, required_configs, tmp_configs);
      if (result == FALSE) return result;
      last_child = last_child->next;
    }
    remove_from_tmp_config(tmp_configs, project_name, act_config);
    
  } else {
    
    // The required configuration of the project_name project is unknown
    boolean found = FALSE; // True if someone requires a configuration about project_name project
    boolean is_active = FALSE;
    
    // Go through every required configuration to check if someone requires
    // something about project_name project.
    struct config_struct* last = all_configs;
    while (last && last->requirements != NULL) {
      struct string2_list* req_config = last->requirements;
      while (req_config && req_config->str1 != NULL && req_config->str2 != NULL) {
        // If someone requires something about project_name project
        if (strcmp(req_config->str1, project_name) == 0) {
          const struct config_struct* tmp = all_configs;
          if (project_config == NULL) {
            // Get the active configuration of project_name project
            while (tmp && tmp->project_name != NULL && tmp->project_conf != NULL) {
              if (strcmp(tmp->project_name, project_name) == 0 && tmp->is_active == TRUE) {
                act_config = tmp->project_conf;
                is_active = TRUE;
                break;
              }
              tmp = tmp->next;
            }
          } else {
            act_config = project_config;
          }
          found = TRUE;
          
          // Insert the project_name with its active configuration
          result = insert_to_tmp_config(tmp_configs, project_name, act_config, is_active);
          if (result == FALSE) {
            return FALSE;
          } else if (result == 2) { // result is 2 if a circle is detected (this will cause error later)
            result = insert_to_required_config(all_configs, project_name, act_config, required_configs);
            if (result == FALSE) return result;
          }
          
          // Analyse the project that requires something about project_name project
          result = analyse_child(all_configs, last->project_name, last->project_conf, required_configs, tmp_configs);
          remove_from_tmp_config(tmp_configs, project_name, act_config);
          // If some errors happened during the analysis of last->project_conf project
          if (result == FALSE) {
            req_config = req_config->next;
            continue;
          }

          // If we still don't know anything about the project_name project's configuration
          if (get_act_config(required_configs, project_name) == NULL) {
            // Go to the next required config
            req_config = req_config->next;
            continue;
          } else {
            found = TRUE;
          }
          
          // Get the project_name's act_config config_struct (tmp holds it)
          tmp = all_configs;
          while (tmp && tmp->project_name != NULL && tmp->project_conf != NULL) {
            if (strcmp(tmp->project_name, project_name) == 0 && strcmp(tmp->project_conf, act_config) == 0) {
              break;
            }
            tmp = tmp->next;
          }

          // We know for sure that the project_name project's configuration is
          // act_config, so we insert all the required configs of project_name 
          // project's act_config configuration
          struct string2_list* last_proj_config = tmp->requirements;
          while (last_proj_config && last_proj_config->str1 != NULL && last_proj_config->str2 != NULL) {
            result = insert_to_required_config(all_configs, last_proj_config->str1, last_proj_config->str2, required_configs);
            if (result == FALSE) return result;
            last_proj_config = last_proj_config->next;
          }
          insert_to_tmp_config(tmp_configs, project_name, act_config, is_active);
          
          // Analyze referenced projects of project_name project
          struct string_list* last_child = tmp->children;
          while (last_child && last_child->str != NULL) {
            result = analyse_child(all_configs, last_child->str, NULL, required_configs, tmp_configs);
            if (result == FALSE) return result;
            last_child = last_child->next;
          }
          remove_from_tmp_config(tmp_configs, tmp->project_name, tmp->project_conf);
        }
        req_config = req_config->next;
      }
      last = last->next;
    }
     // No one said anything about this project's configuration or we still don't know the configuration
    if (found == FALSE || get_act_config(required_configs, project_name) == NULL) {
      //Get the active configuration of this project
      last = all_configs;
      while (last && last->project_name != NULL && last->project_conf != NULL) {
        if (strcmp(last->project_name, project_name) == 0 && last->is_active) {
          act_config = last->project_conf;
          break;
        }
        last = last->next;
      }
      if (last->processed == TRUE) {
        // We already processed this project
        return TRUE;
      }
      
      last->processed = TRUE;
      // Insert the active configuration to the required_configs
      result = insert_to_required_config(all_configs, project_name, act_config, required_configs);
      if (result == FALSE) return result;
      
      //Insert the project requirements of the project_name project's active configuration
      struct string2_list* last_proj_config = last->requirements;
      while (last_proj_config && last_proj_config->str1 != NULL && last_proj_config->str2 != NULL) {
        struct config_list* tmp_tmp = tmp_configs;
        // Detect circle in all_configs, which is started from last_proj_config->str1 and
        // project_name is an element of the circle
        struct string_list* list = NULL;
        boolean circular = is_circular_dep(all_configs, last_proj_config->str1, last_proj_config->str1, project_name, &list);
        boolean need_circular_error = FALSE;
        // Find the last_proj_config->str1 project in the circle, and 
        // determine if it has parent projects other than that are in the circle
        if (circular) {
          // Find the config struct of last_proj_config->str1
          struct config_struct* tmp2 = all_configs;
          while (tmp2 && tmp2->project_name != NULL) {
            if (strcmp(tmp2->project_name, last_proj_config->str1) == 0) {
              break;
            }
            tmp2 = tmp2->next;
          }
          if (list && tmp2 && tmp2->dependencies != NULL) {
            struct string_list* deps = tmp2->dependencies;
            while (deps && deps->str != NULL) {
              struct string_list* tmp_list = list;
              boolean tmp_error = FALSE;
              while (tmp_list && tmp_list->str != NULL) {
                if (strcmp(tmp_list->str, deps->str) == 0) {
                  tmp_error = TRUE;
                  break;
                }
                tmp_list = tmp_list->next;
              }
              if (tmp_error == FALSE) {
                need_circular_error = TRUE;
                break;
              }
              deps = deps->next;
            }
          }
        }
        free_string_list(list);
        // Go through the tmp_configs to check inconsistency
        while (tmp_tmp && tmp_tmp->str1 != NULL && tmp_tmp->str2 != NULL) {
          if (strcmp(tmp_tmp->str1, last_proj_config->str1) == 0 && 
              strcmp(tmp_tmp->str2, last_proj_config->str2) != 0 && 
              circular && need_circular_error) {
            // Insert the configuration. This will cause an error later.
            insert_to_required_config(all_configs, tmp_tmp->str1, tmp_tmp->str2, required_configs);
            result = FALSE;
          }
          tmp_tmp = tmp_tmp->next;
        }
        // Important to && the result at the end. If the result is FALSE from the
        // while cycle above, then it must remain FALSE
        result = insert_to_required_config(all_configs, last_proj_config->str1, last_proj_config->str2, required_configs) && result;
        if (result == FALSE) return result;
        last_proj_config = last_proj_config->next;
      }
      insert_to_tmp_config(tmp_configs, project_name, act_config, last->is_active);
      // Analyse children of the project_name project
      struct string_list* last_child = last->children;
      while (last_child && last_child->str != NULL) {
        result = analyse_child(all_configs, last_child->str, NULL, required_configs, tmp_configs);
        if (result == FALSE) return result;
        last_child = last_child->next;
      }
      remove_from_tmp_config(tmp_configs, project_name, act_config);
    }
  }
  return result;
}

static tpd_result config_struct_get_required_configs(struct config_struct* const all_configs, struct string2_list* const required_configs, struct config_list** tmp_configs) {
  // Init tmp_configs, which will be used to detect circularity
  *tmp_configs = (struct config_list*)Malloc(sizeof(config_list));
  (*tmp_configs)->str1 = NULL;
  (*tmp_configs)->str2 = NULL;
  (*tmp_configs)->is_active = FALSE;
  (*tmp_configs)->next = NULL;
  
  // Fill up dependencies of the configurations
  config_struct_fillup_deps(all_configs);
  struct config_struct* last = all_configs;
  // The top level project is always the first project and we need the active configuration of the project
  while (last) {
    if (last->project_name != NULL && all_configs->project_name != NULL) {
      if (strcmp(all_configs->project_name, last->project_name) == 0 && last->is_active == TRUE && last->processed == FALSE) {
        boolean result = TRUE;
        
        // Insert the top level project's active configuration to the required configs.
        result = insert_to_required_config(all_configs, last->project_name, last->project_conf, required_configs);
        if (result == FALSE) return TPD_FAILED;
        insert_to_tmp_config(*tmp_configs, last->project_name, last->project_conf, TRUE);
        
        // last variable holds the top level project's active configuration which needed to be analyzed
        // Insert every required config of the top level project active configuration
        struct string2_list* last_proj_config = last->requirements;
        while (last_proj_config && last_proj_config->str1 != NULL && last_proj_config->str2 != NULL) { // todo ezek a null cuccok mindenhova, every param should not be null
          struct string_list* children = last->children;
          // This if allows that a top level project can require an other project's configuration
          // without referencing it.
          
          if (children->str != NULL || strcmp(last_proj_config->str1, last->project_name) == 0) {
            result = insert_to_required_config(all_configs, last_proj_config->str1, last_proj_config->str2, required_configs);
            if (result == FALSE) return TPD_FAILED;
          }
          last_proj_config = last_proj_config->next;
        }
        last->processed = TRUE;
        
        //Analyze the referenced project of the top level project
        struct string_list* last_child = last->children;
        while (last_child && last_child->str != NULL) {
          result = analyse_child(all_configs, last_child->str, NULL, required_configs, *tmp_configs); // todo check if everywhere is handled
          if (result == FALSE) return TPD_FAILED;
          last_child = last_child->next;
        }
        remove_from_tmp_config(*tmp_configs, last->project_name, last->project_conf);
        break; // No more needed
      }
    }
    last = last->next;
  }
  return TPD_SUCCESS;
}

static tpd_result process_tpd_internal(const char **p_tpd_name, char* tpdName, const char *actcfg,
  const char *file_list_path, int *p_argc, char ***p_argv, boolean* p_free_argv,
  int *p_optind, char **p_ets_name, char **p_project_name,
  boolean *p_gflag, boolean *p_sflag, boolean *p_cflag, boolean *p_aflag, boolean *preprocess,
  boolean *p_Rflag, boolean *p_lflag, boolean *p_mflag, boolean *p_Pflag,
  boolean *p_Lflag, boolean recursive, boolean force_overwrite, boolean gen_only_top_level,
  const char *output_file, char** abs_work_dir_p, struct string_list* sub_project_dirs,
  const char* program_name, FILE* prj_graph_fp, struct string2_list* create_symlink_list, struct string_list* ttcn3_prep_includes,
  struct string_list* ttcn3_prep_defines, struct string_list* ttcn3_prep_undefines, struct string_list* prep_includes,
  struct string_list* prep_defines, struct string_list* prep_undefines, char **p_csmode, boolean *p_quflag, boolean* p_dsflag,
  char** cxxcompiler, char** optlevel, char** optflags, boolean* p_dbflag, boolean* p_drflag, boolean* p_dtflag, boolean* p_dxflag,
  boolean* p_djflag, boolean* p_fxflag, boolean* p_doflag, boolean* p_gfflag, boolean* p_lnflag, boolean* p_isflag,
  boolean* p_asflag, boolean* p_swflag, boolean* p_Yflag, boolean* p_Mflag, boolean *p_Eflag, boolean* p_nflag, boolean* p_Nflag,
  boolean* p_diflag, struct string_list* solspeclibs, struct string_list* sol8speclibs,
  struct string_list* linuxspeclibs, struct string_list* freebsdspeclibs, struct string_list* win32speclibs, char** ttcn3prep,
  struct string_list* linkerlibs, struct string_list* additionalObjects, struct string_list* linkerlibsearchp, boolean Vflag, boolean Dflag,
  boolean *p_Zflag, boolean *p_Hflag, char** generatorCommandOutput, struct string2_list* target_placement_list, boolean prefix_workdir, 
  struct string2_list* run_command_list, map<cstring, int>& seen_tpd_files, struct string2_list* required_configs, struct string_list** profiled_file_list,
  const char **search_paths, size_t n_search_paths, char** makefileScript, struct config_struct * const all_configs);

extern "C" tpd_result process_tpd(const char **p_tpd_name, const char *actcfg,
  const char *file_list_path, int *p_argc, char ***p_argv, boolean* p_free_argv,
  int *p_optind, char **p_ets_name, char **p_project_name,
  boolean *p_gflag, boolean *p_sflag, boolean *p_cflag, boolean *p_aflag, boolean *preprocess,
  boolean *p_Rflag, boolean *p_lflag, boolean *p_mflag, boolean *p_Pflag,
  boolean *p_Lflag, boolean recursive, boolean force_overwrite, boolean gen_only_top_level,
  const char *output_file, char** abs_work_dir_p, struct string_list* sub_project_dirs,
  const char* program_name, FILE* prj_graph_fp, struct string2_list* create_symlink_list, struct string_list* ttcn3_prep_includes,
  struct string_list* ttcn3_prep_defines, struct string_list* ttcn3_prep_undefines, struct string_list* prep_includes, 
  struct string_list* prep_defines, struct string_list* prep_undefines, char **p_csmode, boolean *p_quflag, boolean* p_dsflag,
  char** cxxcompiler, char** optlevel, char** optflags, boolean* p_dbflag, boolean* p_drflag, boolean* p_dtflag, boolean* p_dxflag, 
  boolean* p_djflag, boolean* p_fxflag, boolean* p_doflag, boolean* p_gfflag, boolean* p_lnflag, boolean* p_isflag,
  boolean* p_asflag, boolean* p_swflag, boolean* p_Yflag, boolean* p_Mflag, boolean* p_Eflag, boolean* p_nflag, boolean* p_Nflag,
  boolean* p_diflag, struct string_list* solspeclibs, struct string_list* sol8speclibs,
  struct string_list* linuxspeclibs, struct string_list* freebsdspeclibs, struct string_list* win32speclibs, char** ttcn3prep,
  string_list* linkerlibs, string_list* additionalObjects, string_list* linkerlibsearchp, boolean Vflag, boolean Dflag, boolean *p_Zflag,
  boolean *p_Hflag, char** generatorCommandOutput, struct string2_list* target_placement_list, boolean prefix_workdir,
  struct string2_list* run_command_list, struct string2_list* required_configs, struct string_list** profiled_file_list,
  const char **search_paths, size_t n_search_paths, char** makefileScript) {

  map<cstring, int> seen_tpd_files;
  char *tpdName = NULL;
  projGenHelper.setZflag(*p_Zflag);
  projGenHelper.setWflag(prefix_workdir);
  projGenHelper.setHflag(*p_Hflag);

  struct config_struct* all_configs = (struct config_struct*)Malloc(sizeof(struct config_struct));
  config_struct_init(all_configs);
  
  // The first round only collects the configurations about the tpd-s into the
  // all_configs variable. It does not do anything else.
  tpd_result success = process_tpd_internal(p_tpd_name, tpdName,
      actcfg, file_list_path, p_argc, p_argv, p_free_argv, p_optind, p_ets_name, p_project_name,
      p_gflag, p_sflag, p_cflag, p_aflag, preprocess,
      p_Rflag, p_lflag, p_mflag, p_Pflag,
      p_Lflag, recursive, force_overwrite, gen_only_top_level,
      output_file, abs_work_dir_p, sub_project_dirs,
      program_name, prj_graph_fp, create_symlink_list, ttcn3_prep_includes,
      ttcn3_prep_defines, ttcn3_prep_undefines, prep_includes, prep_defines,
      prep_undefines, p_csmode, p_quflag, p_dsflag, cxxcompiler,
      optlevel, optflags, p_dbflag, p_drflag, p_dtflag, p_dxflag, p_djflag,
      p_fxflag, p_doflag, p_gfflag, p_lnflag, p_isflag,
      p_asflag, p_swflag, p_Yflag, p_Mflag, p_Eflag, p_nflag, p_Nflag,
      p_diflag, solspeclibs, sol8speclibs,
      linuxspeclibs, freebsdspeclibs, win32speclibs, ttcn3prep,
      linkerlibs, additionalObjects, linkerlibsearchp, Vflag, Dflag, p_Zflag,
      p_Hflag, generatorCommandOutput, target_placement_list, prefix_workdir, 
      run_command_list, seen_tpd_files, required_configs, profiled_file_list,
      search_paths, n_search_paths, makefileScript, all_configs);
  
  if (success == TPD_SUCCESS) {
    struct config_list* tmp_configs = NULL;
    config_struct_get_required_configs(all_configs, required_configs, &tmp_configs);
    free_config_list(tmp_configs);
    free_config_struct(all_configs);
    all_configs = NULL;
    
    // In the second round every configuration is known for every project in the
    // optimal case. In the not optimal case errors are produced.
    // This round does get the information from the tpd to generate the makefile.
    success = process_tpd_internal(p_tpd_name, tpdName,
      actcfg, file_list_path, p_argc, p_argv, p_free_argv, p_optind, p_ets_name, p_project_name,
      p_gflag, p_sflag, p_cflag, p_aflag, preprocess,
      p_Rflag, p_lflag, p_mflag, p_Pflag,
      p_Lflag, recursive, force_overwrite, gen_only_top_level,
      output_file, abs_work_dir_p, sub_project_dirs,
      program_name, prj_graph_fp, create_symlink_list, ttcn3_prep_includes,
      ttcn3_prep_defines, ttcn3_prep_undefines, prep_includes, prep_defines,
      prep_undefines, p_csmode, p_quflag, p_dsflag, cxxcompiler,
      optlevel, optflags, p_dbflag, p_drflag, p_dtflag, p_dxflag, p_djflag,
      p_fxflag, p_doflag, p_gfflag, p_lnflag, p_isflag,
      p_asflag, p_swflag, p_Yflag, p_Mflag, p_Eflag, p_nflag, p_Nflag,
      p_diflag, solspeclibs, sol8speclibs,
      linuxspeclibs, freebsdspeclibs, win32speclibs, ttcn3prep,
      linkerlibs, additionalObjects, linkerlibsearchp, Vflag, Dflag, p_Zflag,
      p_Hflag, generatorCommandOutput, target_placement_list, prefix_workdir, 
      run_command_list, seen_tpd_files, required_configs, profiled_file_list,
      search_paths, n_search_paths, makefileScript, all_configs);
  } else {
    free_config_struct(all_configs);
    all_configs = NULL;
  }
  if (TPD_FAILED == success){
    goto failure;
  }

  if (false == projGenHelper.sanityCheck()) {
    fprintf (stderr, "makefilegen exits\n");
    goto failure;
  }

  projGenHelper.generateRefProjectWorkingDirsTo(*p_project_name);

  for (size_t i = 0, num = seen_tpd_files.size(); i < num; ++i) {
    const cstring& key = seen_tpd_files.get_nth_key(i);
    int *elem = seen_tpd_files.get_nth_elem(i);
    key.destroy();
    delete elem;
  }
  seen_tpd_files.clear();

  return TPD_SUCCESS;
  
failure:
  /* free everything before exiting */
  free_string_list(sub_project_dirs);
  free_string_list(ttcn3_prep_includes);
  free_string_list(ttcn3_prep_defines);
  free_string_list(ttcn3_prep_undefines);
  free_string_list(prep_includes);
  free_string_list(prep_defines);
  free_string_list(prep_undefines);
  free_string_list(solspeclibs);
  free_string_list(sol8speclibs);
  free_string_list(linuxspeclibs);
  free_string_list(freebsdspeclibs);
  free_string_list(win32speclibs);
  free_string_list(linkerlibs);
  free_string_list(additionalObjects);
  free_string_list(linkerlibsearchp);
  free_string_list(*profiled_file_list);

  Free(search_paths);
  Free(*generatorCommandOutput);
  free_string2_list(run_command_list);
  free_string2_list(create_symlink_list);
  free_string2_list(target_placement_list);
  free_string2_list(required_configs);
  Free(*makefileScript);
  Free(*p_project_name);
  Free(*abs_work_dir_p);

  Free(*p_ets_name);
  Free(*cxxcompiler);
  Free(*optlevel);
  Free(*optflags);
  Free(*ttcn3prep);
  if (*p_free_argv) {
    for (int E = 0; E < *p_argc; ++E) Free((*p_argv)[E]);
    Free(*p_argv);
  }
  
  for (size_t i = 0, num = seen_tpd_files.size(); i < num; ++i) {
    const cstring& key = seen_tpd_files.get_nth_key(i);
    int *elem = seen_tpd_files.get_nth_elem(i);
    key.destroy();
    delete elem;
  }
  seen_tpd_files.clear();
  
  return TPD_FAILED;
}

// optind is the index of the next element of argv to be processed.
// Return TPD_SUCESS if parsing successful, TPD_SKIPPED if the tpd was
// seen already, or TPD_FAILED.
//
// Note: if process_tpd() returns TPD_SUCCESS, it is expected that all strings
// (argv[], ets_name, other_files[], output_file) are allocated on the heap
// and need to be freed. On input, these strings point into argv.
// process_tpd() may alter these strings; new values will be on the heap.
// If process_tpd() preserves the content of such a string (e.g. ets_name),
// it must nevertheless make a copy on the heap via mcopystr().
static tpd_result process_tpd_internal(const char **p_tpd_name, char *tpdName, const char *actcfg,
  const char *file_list_path, int *p_argc, char ***p_argv, boolean* p_free_argv,
  int *p_optind, char **p_ets_name, char **p_project_name,
  boolean *p_gflag, boolean *p_sflag, boolean *p_cflag, boolean *p_aflag, boolean *preprocess,
  boolean *p_Rflag, boolean *p_lflag, boolean *p_mflag, boolean *p_Pflag,
  boolean *p_Lflag, boolean recursive, boolean force_overwrite, boolean gen_only_top_level,
  const char *output_file, char** abs_work_dir_p, struct string_list* sub_project_dirs,
  const char* program_name, FILE* prj_graph_fp, struct string2_list* create_symlink_list, struct string_list* ttcn3_prep_includes,
  struct string_list* ttcn3_prep_defines, struct string_list* ttcn3_prep_undefines, struct string_list* prep_includes,
  struct string_list* prep_defines, struct string_list* prep_undefines, char **p_csmode, boolean *p_quflag, boolean* p_dsflag,
  char** cxxcompiler, char** optlevel, char** optflags, boolean* p_dbflag, boolean* p_drflag, boolean* p_dtflag, boolean* p_dxflag,
  boolean* p_djflag, boolean* p_fxflag, boolean* p_doflag, boolean* p_gfflag, boolean* p_lnflag, boolean* p_isflag,
  boolean* p_asflag, boolean* p_swflag, boolean* p_Yflag, boolean* p_Mflag, boolean* p_Eflag, boolean* p_nflag, boolean* p_Nflag,
  boolean* p_diflag, struct string_list* solspeclibs, struct string_list* sol8speclibs,
  struct string_list* linuxspeclibs, struct string_list* freebsdspeclibs, struct string_list* win32speclibs, char** ttcn3prep,
  string_list* linkerlibs, string_list* additionalObjects, string_list* linkerlibsearchp, boolean Vflag, boolean Dflag, boolean *p_Zflag,
  boolean *p_Hflag, char** generatorCommandOutput, struct string2_list* target_placement_list, boolean prefix_workdir,
  struct string2_list* run_command_list, map<cstring, int>& seen_tpd_files, struct string2_list* required_configs, struct string_list** profiled_file_list,
  const char **search_paths, size_t n_search_paths, char** makefileScript, struct config_struct * const all_configs)
{
  tpd_result result = TPD_SUCCESS;
  // read-only non-pointer aliases
  //char** const& local_argv = *p_argv;
  int const& local_argc = *p_argc;
  int const& local_optind = *p_optind;
  *abs_work_dir_p = NULL;
  const boolean get_config_mode = all_configs != NULL;

  assert(local_optind >= 2 // at least '-ttpd_name' must be in the args
    || local_optind == 0); // if called for a referenced project

  assert(local_argc   >= local_optind);
  
  autostring tpd_dir(get_dir_from_path(*p_tpd_name));
  autostring abs_tpd_dir(get_absolute_dir(tpd_dir, NULL, FALSE));
  struct stat buf;
  //Only referenced project, when first try is failed,              and when not absolute path
  if(0 == local_optind && *p_tpd_name != NULL && stat(*p_tpd_name, &buf) && tpdName != NULL) {
    //Find the first search_path that has the tpd
    for(size_t i = 0; i<n_search_paths; i++) {
      boolean need_slash = search_paths[i][strlen(search_paths[i]) - 1] != '/';
      NOTIFY("Cannot find %s, trying with %s%s%s\n", *p_tpd_name, search_paths[i], need_slash ? "/" : "", tpdName);
      //Create path
      char * prefixed_file_path = (char*)Malloc((strlen(search_paths[i]) + strlen(tpdName) + 1 + need_slash) * sizeof(char));
      strcpy(prefixed_file_path, search_paths[i]);
      if(need_slash) {
        strcat(prefixed_file_path, "/");
      }
      strcat(prefixed_file_path, tpdName);

      tpd_dir = get_dir_from_path(prefixed_file_path);
      abs_tpd_dir = get_absolute_dir(tpd_dir, NULL, FALSE);

      if(!stat(prefixed_file_path, &buf)){
        //Ok, tpd found
        Free(const_cast<char*>(*p_tpd_name));
        *p_tpd_name = prefixed_file_path;
        NOTIFY("TPD with name %s found at %s.", tpdName, search_paths[i]);
        break;
      } else {
        //tpd not found, continue search
        abs_tpd_dir = NULL;
        Free(prefixed_file_path);
      }
    } 
    //Error if tpd is not found in either search paths
    if(NULL == (const char*)abs_tpd_dir) {
      //Only write out the name in the error message (without .tpd)
      tpdName[strlen(tpdName)-4] ='\0';
      ERROR("Unable to find ReferencedProject with name: %s", (const char*)tpdName);
      return TPD_FAILED;
    }
  }

  if (NULL == (const char*)abs_tpd_dir) {
    ERROR("absolute TPD directory could not be retrieved from %s", (const char*)tpd_dir);
    return TPD_FAILED;
  }
  autostring tpd_filename(get_file_from_path(*p_tpd_name));
  autostring abs_tpd_name(compose_path_name(abs_tpd_dir, tpd_filename));
  
  if (local_optind == 0) {
    Free(const_cast<char*>(*p_tpd_name));
    *p_tpd_name = mcopystr((const char*)abs_tpd_name);
  }
    
  if (seen_tpd_files.has_key(abs_tpd_name)) {
    ++*seen_tpd_files[abs_tpd_name];
    return TPD_SKIPPED; // nothing to do
  }
  else {
    if (recursive && !prefix_workdir) {
      // check that this tpd file is not inside a directory of another tpd file
      for (size_t i = 0; i < seen_tpd_files.size(); ++i) {
        const cstring& other_tpd_name = seen_tpd_files.get_nth_key(i);
        autostring other_tpd_dir(get_dir_from_path((const char*)other_tpd_name));
        if (strcmp((const char*)abs_tpd_dir,(const char*)other_tpd_dir)==0) {
          ERROR("TPD files `%s' and `%s' are in the same directory! Use the `-W' option.", (const char*)abs_tpd_name, (const char*)other_tpd_name);
          return TPD_FAILED;
        }
      }
    }
    // mcopystr makes another copy for the map
    seen_tpd_files.add(cstring(mcopystr(abs_tpd_name)), new int(1));
  }

  vector<char> base_files; // values Malloc'd but we pass them to the caller

  XmlDoc doc(xmlParseFile(*p_tpd_name));
  if (doc == NULL) {
    fprintf(stderr, "Error: unable to parse file \"%s\"\n", *p_tpd_name);
    return TPD_FAILED;
  }

  if (!Vflag && get_config_mode) {
    // try schema validation if tpd schema file was found
    bool tpd_is_valid = false;
    const char* ttcn3_dir = getenv("TTCN3_DIR");
    if (ttcn3_dir) {
      size_t ttcn3_dir_len = strlen(ttcn3_dir);
      bool ends_with_slash = (ttcn3_dir_len>0) && (ttcn3_dir[ttcn3_dir_len - 1]=='/');
      expstring_t xsd_file_name = mprintf("%s%setc/xsd/TPD.xsd", ttcn3_dir, ends_with_slash?"":"/");
      autostring xsd_file_name_as(xsd_file_name);
      if (get_path_status(xsd_file_name)==PS_FILE) {
        if (validate_tpd(doc, *p_tpd_name, xsd_file_name)) {
          tpd_is_valid = true;
          NOTIFY("TPD file `%s' validated successfully with schema file `%s'", *p_tpd_name, xsd_file_name);
        }
      } else {
        ERROR("Cannot find XSD for schema for validation of TPD on path `%s'", xsd_file_name);
      }
    } else {
      ERROR("Environment variable TTCN3_DIR not present, cannot find XSD for schema validation of TPD");
    }
    if (!tpd_is_valid) {
      return TPD_FAILED;
    }
  }

  // Source files.
  // Key is projectRelativePath, value is relativeURI or rawURI.
  map<cstring, const char> files;   // values Malloc'd
  map<cstring, const char> folders; // values Malloc'd
  // NOTE! files and folders must be local variables of process_tpd.
  // This is because the keys (not the values) are owned by the XmlDoc.

  map<cstring, const char> path_vars;
  
  autostring workdir;
  
  autostring proj_abs_workdir;

  autostring abs_workdir;

  XPathContext xpathCtx(xmlXPathNewContext(doc));
  if (xpathCtx == NULL) {
    fprintf(stderr,"Error: unable to create new XPath context\n");
    return TPD_FAILED;
  }
  
    /////////////////////////////////////////////////////////////////////////////
  {
    char *projectNameXpath = mprintf("/TITAN_Project_File_Information/ProjectName/text()");
    XPathObject projectNameObj(run_xpath(xpathCtx, projectNameXpath));
    Free(projectNameXpath);
    if (projectNameObj->nodesetval && projectNameObj->nodesetval->nodeNr > 0) {
      if (*p_project_name != NULL) {
        Free(*p_project_name);
      }
      *p_project_name = mcopystr((const char*)projectNameObj->nodesetval->nodeTab[0]->content);
      projGenHelper.addTarget(*p_project_name);
      projGenHelper.setToplevelProjectName(*p_project_name);
      ProjectDescriptor* projDesc = projGenHelper.getTargetOfProject(*p_project_name);
      if (projDesc) projDesc->setProjectAbsTpdDir((const char*)abs_tpd_dir);
      projGenHelper.insertAndCheckProjectName((const char*)abs_tpd_name, *p_project_name);
    }
  }
  
  /////////////////////////////////////////////////////////////////////////////
  
  if (!actcfg && !get_config_mode) {
    actcfg = get_act_config(required_configs, *p_project_name);
  }

  if (actcfg == NULL) {
    // Find out the active config
    XPathObject  activeConfig(run_xpath(xpathCtx,
      "/TITAN_Project_File_Information/ActiveConfiguration/text()"));
    if (activeConfig->nodesetval && activeConfig->nodesetval->nodeNr == 1) {
      // there is one node
      actcfg = (const char*)activeConfig->nodesetval->nodeTab[0]->content;
    }
  }
  
  if (actcfg == NULL) {
    ERROR("Can not find the active build configuration.");
    for (size_t i = 0; i < folders.size(); ++i) {
      Free(const_cast<char*>(folders.get_nth_elem(i)));
    }
    folders.clear();
    return TPD_FAILED;
  }

  { // check if the active configuration exists
    expstring_t xpathActCfg= mprintf(
      "/TITAN_Project_File_Information/Configurations/"
        "Configuration[@name='%s']/text()", actcfg);
    XPathObject theConfigEx(run_xpath(xpathCtx, xpathActCfg));
    Free(xpathActCfg);

    xmlNodeSetPtr nodes = theConfigEx->nodesetval;
    if (nodes == NULL) {
      ERROR("The active build configuration named '%s' of project '%s' does not exist",
        actcfg, *p_project_name);
      for (size_t i = 0; i < folders.size(); ++i) {
        Free(const_cast<char*>(folders.get_nth_elem(i)));
      }
      folders.clear();
      return TPD_FAILED;
    }
  }

  if (!get_config_mode) {
    {
      struct string2_list* last_elem = required_configs;
      //                        To ensure that the first elem is checked too if last_elem->next is null
      while (last_elem && last_elem->str1 != NULL && last_elem->str2 != NULL) {
        if (!strcmp(last_elem->str1, *p_project_name) && strcmp(last_elem->str2, actcfg)) {
          { // check if the other configuration exists
            expstring_t xpathActCfg= mprintf(
              "/TITAN_Project_File_Information/Configurations/"
                "Configuration[@name='%s']/text()", last_elem->str2);
            XPathObject theConfigEx(run_xpath(xpathCtx, xpathActCfg));
            Free(xpathActCfg);

            xmlNodeSetPtr nodes = theConfigEx->nodesetval;
            if (nodes == NULL) {
              ERROR("The active build configuration named '%s' of project '%s' does not exist",
                last_elem->str2, *p_project_name);
              for (size_t i = 0; i < folders.size(); ++i) {
                Free(const_cast<char*>(folders.get_nth_elem(i)));
              }
              folders.clear();
              return TPD_FAILED;
            }
          }
          ERROR("Required configuration is inconsistent or circular : Project '%s' cannot have 2 "
                "different configuration '%s' and '%s'",
                last_elem->str1, actcfg, last_elem->str2);
          for (size_t i = 0; i < folders.size(); ++i) {
            Free(const_cast<char*>(folders.get_nth_elem(i)));
          }
          folders.clear();
          return TPD_FAILED;
        }
        last_elem = last_elem->next;
      }
    }

  // Collect path variables
  {
    XPathObject pathsObj(run_xpath(xpathCtx,
      "/TITAN_Project_File_Information/PathVariables/PathVariable"));
    xmlNodeSetPtr nodes = pathsObj->nodesetval;

    const char *name = 0, *value = 0;
    if (nodes) for (int i = 0; i < nodes->nodeNr; ++i) {
      // nodes->nodeTab[i]->name === "PathVariable"
      for (xmlAttrPtr attr = nodes->nodeTab[i]->properties; attr; attr = attr->next) {
        if (!strcmp((const char*)attr->name, "name")) {
          name = (const char*)attr->children->content;
        }
        else if (!strcmp((const char*)attr->name, "value")) {
          value = (const char*)attr->children->content;
        }
        else {
          WARNING("Unknown attribute %s", (const char*)nodes->nodeTab[i]->name);
        }
      } // next attribute

      if (name && value) path_vars.add(cstring(name), value);
      else ERROR("A PathVariable must have both name and value");
    } // next PathVariable
  }

  // Collect folders
  {
    XPathObject foldersObj(run_xpath(xpathCtx,
      "/TITAN_Project_File_Information/Folders/FolderResource"));

    xmlNodeSetPtr nodes = foldersObj->nodesetval;
    if (nodes) for (int i = 0; i < nodes->nodeNr; ++i) {
      // nodes->nodeTab[i]->name === "FolderResource"
      const char *uri = 0, *path = 0, *raw = 0;

      // projectRelativePath is the path as it appears in Project Explorer (illusion)
      // relativeURI is the actual location, relative to the project root (reality)
      // rawURI is present if the relative path can not be calculated
      //
      // Theoretically these attributes could be in any order, loop over them
      for (xmlAttrPtr attr = nodes->nodeTab[i]->properties; attr; attr = attr->next) {
        if (!strcmp((const char*)attr->name, "projectRelativePath")) {
          path = (const char*)attr->children->content;
        }
        else if (!strcmp((const char*)attr->name, "relativeURI")) {
          uri = (const char*)attr->children->content;
        }
        else if (!strcmp((const char*)attr->name, "rawURI")) {
          raw = (const char*)attr->children->content;
        }
        else {
          WARNING("Unknown attribute %s", (const char*)nodes->nodeTab[i]->name);
        }
      } // next attribute

      if (path == NULL) {
        ERROR("A FolderResource must have a projectRelativePath");
        continue;
      }

      if (uri == NULL && raw == NULL) {
        ERROR("A FolderResource must have either relativeURI or rawURI");
        continue;
      }
      // relativeURI wins over rawURI
      folders.add(cstring(path), uri ? mcopystr(uri) : cook(raw, path_vars));
      // TODO uri: cut "file:", complain on anything else
    } // next FolderResource
  }

  // working directory stuff
  {
    const char* workdirFromTpd = "bin"; // default value
    char *workdirXpath = mprintf(
      "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
      "/ProjectProperties/LocalBuildSettings/workingDirectory/text()",
      actcfg);
    XPathObject workdirObj(run_xpath(xpathCtx, workdirXpath));
    Free(workdirXpath);
    if (workdirObj->nodesetval && workdirObj->nodesetval->nodeNr > 0) {
      workdirFromTpd = (const char*)workdirObj->nodesetval->nodeTab[0]->content;
    }
    if (prefix_workdir) { // the working directory is: prjNameStr + "_" + workdirFromTpd
      const char* prjNameStr = "unnamedproject";
      XPathObject prjName(run_xpath(xpathCtx, "/TITAN_Project_File_Information/ProjectName/text()"));
      if (prjName->nodesetval && prjName->nodesetval->nodeNr == 1) {
        prjNameStr = (const char*)prjName->nodesetval->nodeTab[0]->content;
      }
      workdir = mprintf("%s_%s", prjNameStr, workdirFromTpd);
    } else {
      workdir = mcopystr(workdirFromTpd);
    }
  }
  if (!folders.has_key(workdir)) {
    // Maybe the tpd was saved with the option "No info about work dir"
    folders.add(workdir, mcopystr(workdir)); // fake it
  }
  const char *real_workdir = folders[workdir]; // This is relative to the location of the tpd file
  excluded_folders.add(real_workdir); // excluded by convention

  // If -D flag was specified then we ignore the workdir
  // in the TPD (the current dir is considered the work dir).
  if (!Dflag) {
    bool hasWorkDir = false;
    // if the working directory does not exist create it
    autostring saved_work_dir(get_working_dir());
    if (set_working_dir(abs_tpd_dir)) {
      ERROR("Could not change to project directory `%s'", (const char*)abs_tpd_dir);
    } else {
      switch (get_path_status(real_workdir)) {
      case PS_FILE:
        ERROR("Cannot create working directory `%s' in project directory `%s' because a file with the same name exists", (const char*)abs_tpd_dir, real_workdir);
        break;
      case PS_DIRECTORY:
        // already exists
        hasWorkDir = true;
        break;
      default:
        if (recursive || local_argc != 0) { // we only want to create workdir if necessary
          fprintf(stderr, "Working directory `%s' in project `%s' does not exist, trying to create it...\n",
                  real_workdir, (const char*)abs_tpd_dir);
          int rv = mkdir(real_workdir, 0755);
          if (rv) ERROR("Could not create working directory, mkdir() failed: %s", strerror(errno));
          else printf("Working directory created\n");
          hasWorkDir = true;
        }
      }
    }

    if (local_argc==0) { // if not top level
      set_working_dir(saved_work_dir); // restore working directory
    } else { // if top level
      set_working_dir(real_workdir); // go into the working dir
    }
    if (hasWorkDir) { //we created working directory, or its already been created (from a parent makefilegen process maybe)
      *abs_work_dir_p = get_absolute_dir(real_workdir, abs_tpd_dir, TRUE);
      abs_workdir = (mcopystr(*abs_work_dir_p));
      proj_abs_workdir = mcopystr(*abs_work_dir_p);
    }
  }

  if (Dflag) { // the path to subproject working dir is needed to find the linkerlibsearchpath
    proj_abs_workdir = compose_path_name(abs_tpd_dir, real_workdir);
  }

  ProjectDescriptor* projDesc = projGenHelper.getTargetOfProject(*p_project_name);
  if (projDesc) {
    projDesc->setProjectAbsWorkingDir((const char*)proj_abs_workdir);
    projDesc->setProjectWorkingDir(real_workdir);
    projDesc->setTPDFileName(*p_tpd_name);
  }

  /////////////////////////////////////////////////////////////////////////////

  // Gather the excluded folders in the active config
  {
    expstring_t xpathActCfgPaths = mprintf(
      "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
      "/FolderProperties/FolderResource/FolderProperties/ExcludeFromBuild[text()='true']"
      // This was the selection criterium, we need to go up and down for the actual information
      "/parent::*/parent::*/FolderPath/text()",
      actcfg);
    XPathObject theConfigEx(run_xpath(xpathCtx, xpathActCfgPaths));
    Free(xpathActCfgPaths);

    xmlNodeSetPtr nodes = theConfigEx->nodesetval;
    if (nodes) for (int i = 0; i < nodes->nodeNr; ++i) {

      excluded_folders.add((const char*)nodes->nodeTab[i]->content);
    }
  }

  // Gather individual excluded files in the active config
  {
    expstring_t xpathActCfgPaths = mprintf(
      "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
      "/FileProperties/FileResource/FileProperties/ExcludeFromBuild[text()='true']"
      "/parent::*/parent::*/FilePath/text()",
      actcfg);
    XPathObject theConfigEx(run_xpath(xpathCtx, xpathActCfgPaths));
    Free(xpathActCfgPaths);

    xmlNodeSetPtr nodes = theConfigEx->nodesetval;
    if (nodes) for (int i = 0; i < nodes->nodeNr; ++i) {
      xmlNodePtr curnode = nodes->nodeTab[i];
      cstring aa((const char*)curnode->content);
      if (!excluded_files.has_key(aa)) {
        excluded_files.add(aa, *p_project_name);
      } else {
        WARNING("Multiple exclusion of file %s", (const char*)curnode->content);
      }
    }
  }

  // Collect files; filter out excluded ones
  {
    XPathObject  filesObj(run_xpath(xpathCtx,
      "TITAN_Project_File_Information/Files/FileResource"));

    xmlNodeSetPtr nodes = filesObj->nodesetval;
    if (nodes) for (int i = 0; i < nodes->nodeNr; ++i) {
      // nodes->nodeTab[i]->name === "FileResource"
      const char *uri = 0, *path = 0, *raw = 0;

      // projectRelativePath is the path as it appears in Project Explorer (illusion)
      // relativeURI is the actual location, relative to the project root (reality)
      // rawURI is present if the relative path can not be calculated
      //
      // Theoretically these attributes could be in any order, loop over them
      for (xmlAttrPtr attr = nodes->nodeTab[i]->properties; attr; attr = attr->next) {
        if (!strcmp((const char*)attr->name, "projectRelativePath")) {
          path = (const char*)attr->children->content;
        }
        else if (!strcmp((const char*)attr->name, "relativeURI")) {
          uri = (const char*)attr->children->content;
        }
        else if (!strcmp((const char*)attr->name, "rawURI")) {
          raw = (const char*)attr->children->content;
        }
        else {
          WARNING("Unknown attribute %s", (const char*)nodes->nodeTab[i]->name);
        }
      } // next attribute

      if (path == NULL) {
        ERROR("A FileResource must have a projectRelativePath");
        continue;
      }

      if (uri == NULL && raw == NULL) {
        ERROR("A FileResource must have either relativeURI or rawURI");
        continue;
      }

      cstring cpath(path);
      if (!is_excluded_file(cpath, *p_project_name) && !is_excluded_folder(path)) {
        // relativeURI wins over rawURI
        char *ruri = uri ? mcopystr(uri) : cook(raw, path_vars);
        if (files.has_key(cpath)) {
          ERROR("A FileResource %s must be unique!", (const char*)cpath);
        }
        else {
          bool drop = false;
          const char* file_path = ruri;
          expstring_t rel_file_dir = get_dir_from_path(file_path);
          expstring_t file_name = get_file_from_path(file_path);
          expstring_t abs_dir_path = get_absolute_dir(rel_file_dir, abs_tpd_dir, TRUE);
          expstring_t abs_file_name = compose_path_name(abs_dir_path, file_name);
          if (abs_file_name != NULL) {
            if (get_path_status(abs_file_name) == PS_FILE) {
              FILE *fp = fopen(abs_file_name, "r");
              if (fp != NULL) {
                char* ttcn3_module_name;
                if (is_ttcn3_module(abs_file_name, fp, &ttcn3_module_name)) {
                  projGenHelper.addTtcn3ModuleToProject(*p_project_name, ttcn3_module_name);
                }
                Free(ttcn3_module_name);
                char* asn1_module_name;
                if (is_asn1_module(abs_file_name, fp, &asn1_module_name)) {
                  projGenHelper.addAsn1ModuleToProject(*p_project_name, asn1_module_name);
                }
                Free(asn1_module_name);
                char* xsd_module_name = NULL;
                if (is_xsd_module(abs_file_name, &xsd_module_name)) {
                  projGenHelper.addXSDModuleToProject(*p_project_name, xsd_module_name);
                }
                Free(xsd_module_name);
                if (projGenHelper.isCPPSourceFile(file_name)) {
                   projGenHelper.addUserSourceToProject(*p_project_name, file_name);
                }
                if (projGenHelper.isCPPHeaderFile(file_name)) {
                   projGenHelper.addUserHeaderToProject(*p_project_name, file_name);
                }
                if (projGenHelper.isTtcnPPFile(file_name)) {
                   projGenHelper.addTtcnPPToProject(*p_project_name, file_name);
                }
              }
              fclose(fp);
            }else {
              drop = true;
              ERROR("%s does not exist", abs_file_name);
            }
          }
          if(abs_dir_path != NULL && !drop){
            files.add(cpath, ruri); // relativeURI to the TPD location
          }else {
            result = TPD_FAILED;
          }
          { // set the *preprocess value if .ttcnpp file was found
            const size_t ttcnpp_extension_len = 7; // ".ttcnpp"
            const size_t ruri_len = strlen(ruri);
            if ( ruri_len>ttcnpp_extension_len && strcmp(ruri+(ruri_len-ttcnpp_extension_len),".ttcnpp")==0 ) {
              *preprocess = TRUE;
            }
          }
          Free(rel_file_dir);
          Free(file_name);
          Free(abs_dir_path);
          Free(abs_file_name);
        }
      }
    } // next FileResource
  }
  
  // Gather the code splitting mode from the active configuration
  {
    expstring_t xpathActCfgCodeSplitting = mprintf(
      "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
      "//ProjectProperties/MakefileSettings/codeSplitting/text()",
      actcfg);
    XPathObject codeSplittingObj(run_xpath(xpathCtx, xpathActCfgCodeSplitting));
    Free(xpathActCfgCodeSplitting);
    if (codeSplittingObj->nodesetval && codeSplittingObj->nodesetval->nodeNr > 0) {
      const char* content = (const char*)codeSplittingObj->nodesetval->nodeTab[0]->content;
      if (local_argc != 0) { // top level project
        // Get the code splitting without thinking
        *p_csmode = mcopystr(content);
      } else if (*p_csmode == NULL && strcmp(content, "none") != 0) { // todo config in error message?
        ERROR("The top level project does not have code splitting set, but the `%s' project has `%s' code splitting set.",
          *p_project_name, content);
      } else if (*p_csmode != NULL && strcmp(content, *p_csmode)) {
        ERROR("Code splitting must be the same. Top level project has `%s', `%s' project has `%s' code splitting set.",
          *p_csmode, *p_project_name, content);
      }
    } else if (*p_csmode != NULL && strcmp(*p_csmode, "none") != 0) {
      ERROR("The top level project have `%s' code splitting set, but the `%s' project has none.",
        *p_csmode, *p_project_name);
    }
  }

  // Check options
  xsdbool2boolean(xpathCtx, actcfg, "useAbsolutePath", p_aflag);
  xsdbool2boolean(xpathCtx, actcfg, "GNUMake", p_gflag);
  if (*p_Zflag) *p_lflag = FALSE;
  xsdbool2boolean(xpathCtx, actcfg, "dynamicLinking", p_lflag);
  xsdbool2boolean(xpathCtx, actcfg, "functiontestRuntime", p_Rflag);
  xsdbool2boolean(xpathCtx, actcfg, "singleMode", p_sflag);
  xsdbool2boolean(xpathCtx, actcfg, "quietly", p_quflag);
  xsdbool2boolean(xpathCtx, actcfg, "disableSubtypeChecking", p_dsflag);
  xsdbool2boolean(xpathCtx, actcfg, "disableBER", p_dbflag);
  xsdbool2boolean(xpathCtx, actcfg, "disableRAW", p_drflag);
  xsdbool2boolean(xpathCtx, actcfg, "disableTEXT", p_dtflag);
  xsdbool2boolean(xpathCtx, actcfg, "disableXER", p_dxflag);
  xsdbool2boolean(xpathCtx, actcfg, "disableJSON", p_djflag);
  xsdbool2boolean(xpathCtx, actcfg, "forceXERinASN.1", p_fxflag);
  xsdbool2boolean(xpathCtx, actcfg, "defaultasOmit", p_doflag);
  xsdbool2boolean(xpathCtx, actcfg, "gccMessageFormat", p_gfflag);
  xsdbool2boolean(xpathCtx, actcfg, "lineNumbersOnlyInMessages", p_lnflag);
  xsdbool2boolean(xpathCtx, actcfg, "includeSourceInfo", p_isflag);
  xsdbool2boolean(xpathCtx, actcfg, "addSourceLineInfo", p_asflag);
  xsdbool2boolean(xpathCtx, actcfg, "suppressWarnings", p_swflag);
  xsdbool2boolean(xpathCtx, actcfg, "outParamBoundness", p_Yflag); //not documented, obsolete
  xsdbool2boolean(xpathCtx, actcfg, "forceOldFuncOutParHandling", p_Yflag);
  xsdbool2boolean(xpathCtx, actcfg, "omitInValueList", p_Mflag);
  xsdbool2boolean(xpathCtx, actcfg, "warningsForBadVariants", p_Eflag);
  xsdbool2boolean(xpathCtx, actcfg, "activateDebugger", p_nflag);
  xsdbool2boolean(xpathCtx, actcfg, "disablePredefinedExternalFolder", p_diflag);
  xsdbool2boolean(xpathCtx, actcfg, "ignoreUntaggedOnTopLevelUnion", p_Nflag);

  projDesc = projGenHelper.getTargetOfProject(*p_project_name);
  if (projDesc) projDesc->setLinkingStrategy(*p_lflag);

  // Extract the "incremental dependencies" option
  {
    boolean incremental_deps = TRUE;
    xsdbool2boolean(xpathCtx, actcfg, "incrementalDependencyRefresh", &incremental_deps);

    // For makefilegen, "Use GNU make" implies incremental deps by default,
    // unless explicitly disabled by "use makedepend" (a.k.a. mflag).
    // For Eclipse, incremental deps must be explicitly specified,
    // even if GNU make is being used.
    
    if (incremental_deps) {
      if( !(*p_gflag) ) {
        WARNING("Incremental dependency ordered but it requires gnu make");
      }
    }
    else {
      if (*p_gflag) {
        // GNU make but no incremental deps
        *p_mflag = true;
      }
    }
  }
  
  // Extract the makefileScript only for top level
  // In the recursive case the subsequent makefile calls will process the 
  // makefileScript
  if (local_argc != 0)
  {
    char *makefileScriptXpath = mprintf(
      "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
      "/ProjectProperties/LocalBuildSettings/MakefileScript/text()",
      actcfg);
    XPathObject makefileScriptObj(run_xpath(xpathCtx, makefileScriptXpath));
    Free(makefileScriptXpath);
    if (makefileScriptObj->nodesetval && makefileScriptObj->nodesetval->nodeNr > 0) {
      const char* file_path = (const char*)makefileScriptObj->nodesetval->nodeTab[0]->content;
      expstring_t rel_file_dir = get_dir_from_path(file_path);
      expstring_t file_name = get_file_from_path(file_path);
      expstring_t abs_dir_path = get_absolute_dir(rel_file_dir, abs_tpd_dir, TRUE);
      *makefileScript = compose_path_name(abs_dir_path, file_name);
      Free(rel_file_dir);
      Free(file_name);
      Free(abs_dir_path);
    }
  }

  // Extract the default target option
  // if it is not defined as a command line argument
  if (!(*p_Lflag)) {
    expstring_t defTargetXpath = mprintf(
      "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
      "/ProjectProperties/MakefileSettings/defaultTarget/text()",
      actcfg);
    XPathObject defTargetObj(run_xpath(xpathCtx, defTargetXpath));
    Free(defTargetXpath);
    if (defTargetObj->nodesetval && defTargetObj->nodesetval->nodeNr > 0) {
      const char* content = (const char*)defTargetObj->nodesetval->nodeTab[0]->content;
      if (!strcmp(content, "library")) {
        *p_Lflag = true;
      } else if (!strcmp(content, "executable")) {
        *p_Lflag = false;
      } else {
        ERROR("Unknown default target: '%s'."
            " The available targets are: 'executable', 'library'", content);
      }
    }
    ProjectDescriptor* projDescr = projGenHelper.getTargetOfProject(*p_project_name);
    if (projDescr) projDescr->setLibrary(*p_Lflag);
  }

  // Executable name (don't care unless top-level invocation)
  if (local_argc != 0)
  {
    char *exeXpath = mprintf(
      "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
      "/ProjectProperties/MakefileSettings/targetExecutable/text()",
      actcfg);
    XPathObject exeObj(run_xpath(xpathCtx, exeXpath));
    Free(exeXpath);
    if (exeObj->nodesetval && exeObj->nodesetval->nodeNr > 0) {
      const char* target_executable = (const char*)exeObj->nodesetval->nodeTab[0]->content;
      autostring target_exe_dir(get_dir_from_path(target_executable));
      autostring target_exe_file(get_file_from_path(target_executable));
      if (target_exe_dir!=NULL) { // if it's not only a file name
        if (get_path_status(target_exe_dir)==PS_NONEXISTENT) {
          if (strcmp(real_workdir,target_exe_dir)!=0) {
            WARNING("Provided targetExecutable directory `%s' does not exist, only file name `%s' will be used", (const char*)target_exe_dir, (const char*)target_exe_file);
          }
          target_executable = target_exe_file;
        }
      }
      if (!*p_ets_name) { // Command line will win
        *p_ets_name = mcopystr(target_executable);
      }
    }
  }

  // create an xml element for the currently processed project
  if (prj_graph_fp) {
    const char* prjNameStr = "???";
    XPathObject  prjName(run_xpath(xpathCtx, "/TITAN_Project_File_Information/ProjectName/text()"));
    if (prjName->nodesetval && prjName->nodesetval->nodeNr == 1) {
      prjNameStr = (const char*)prjName->nodesetval->nodeTab[0]->content;
    }
    autostring tpd_rel_dir(get_relative_dir(tpd_dir, NULL));
    autostring tpd_rel_path(compose_path_name(tpd_rel_dir, (const char*)tpd_filename));
    fprintf(prj_graph_fp, "<project name=\"%s\" uri=\"%s\">\n", prjNameStr, (const char*)tpd_rel_path);
    XPathObject subprojects(run_xpath(xpathCtx, "/TITAN_Project_File_Information/ReferencedProjects/ReferencedProject/attribute::name"));
    xmlNodeSetPtr nodes = subprojects->nodesetval;
    if (nodes) for (int i = 0; i < nodes->nodeNr; ++i) {
      const char* refd_name = "???";
      if (!strcmp((const char*)nodes->nodeTab[i]->name, "name")) {
        refd_name = (const char*)nodes->nodeTab[i]->children->content;
      }
      fprintf(prj_graph_fp, "<reference name=\"%s\"/>\n", refd_name);
    }
    fprintf(prj_graph_fp, "</project>\n");
  }

  // Tpd part of the MakefileSettings
  {
    //TTCN3preprocessorIncludes
    char *preincludeXpath = mprintf(
        "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
        "/ProjectProperties/MakefileSettings/TTCN3preprocessorIncludes/listItem/text()",
        actcfg);
    XPathObject preincludeObj(run_xpath(xpathCtx, preincludeXpath));
    Free(preincludeXpath);

    xmlNodeSetPtr nodes = preincludeObj->nodesetval;

    if (nodes) for (int i = 0; i < nodes->nodeNr; ++i) {
      char* content = (char*)preincludeObj->nodesetval->nodeTab[i]->content;

      // add includes to the end of list
      if (ttcn3_prep_includes) {
        // go to last element
        struct string_list* last_elem = ttcn3_prep_includes;
        while (last_elem->next) last_elem = last_elem->next;
        // add string to last element if empty or create new last element and add it to that
        if (last_elem->str) {
          last_elem->next = (struct string_list*)Malloc(sizeof(struct string_list));
          last_elem = last_elem->next;
          last_elem->next = NULL;
        }
        replacechar(&content);
        last_elem->str = content;
      }
    }
  }
  {
    //TTCN3preprocessorDefines
    char *ttcn3predefinesXpath = mprintf(
        "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
        "/ProjectProperties/MakefileSettings/TTCN3preprocessorDefines/listItem/text()",
        actcfg);
    XPathObject ttcn3predefinesObj(run_xpath(xpathCtx, ttcn3predefinesXpath));
    Free(ttcn3predefinesXpath);

    xmlNodeSetPtr nodes = ttcn3predefinesObj->nodesetval;

    if (nodes) for (int i = 0; i < nodes->nodeNr; ++i) {
      const char* content = (const char*)ttcn3predefinesObj->nodesetval->nodeTab[i]->content;

      // add includes to the end of list
      if (ttcn3_prep_defines) {
        // go to last element
        struct string_list* last_elem = ttcn3_prep_defines;
        while (last_elem->next) last_elem = last_elem->next;
        // add string to last element if empty or create new last element and add it to that
        if (last_elem->str) {
          last_elem->next = (struct string_list*)Malloc(sizeof(struct string_list));
          last_elem = last_elem->next;
          last_elem->next = NULL;
        }
        last_elem->str = mcopystr(content);
      }
    }
  }
  {
    //TTCN3preprocessorUnDefines
    char *ttcn3preUndefinesXpath = mprintf(
        "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
        "/ProjectProperties/MakefileSettings/TTCN3preprocessorUndefines/listItem/text()",
        actcfg);
    XPathObject ttcn3preUndefinesObj(run_xpath(xpathCtx, ttcn3preUndefinesXpath));
    Free(ttcn3preUndefinesXpath);

    xmlNodeSetPtr nodes = ttcn3preUndefinesObj->nodesetval;

    if (nodes) for (int i = 0; i < nodes->nodeNr; ++i) {
      const char* content = (const char*)ttcn3preUndefinesObj->nodesetval->nodeTab[i]->content;

      // add includes to the end of list
      if (ttcn3_prep_undefines) {
        // go to last element
        struct string_list* last_elem = ttcn3_prep_undefines;
        while (last_elem->next) last_elem = last_elem->next;
        // add string to last element if empty or create new last element and add it to that
        if (last_elem->str) {
          last_elem->next = (struct string_list*)Malloc(sizeof(struct string_list));
          last_elem = last_elem->next;
          last_elem->next = NULL;
        }
        last_elem->str = mcopystr(content);
      }
    }
  }

  {
    //preprocessorIncludes
    char *preincludesXpath = mprintf(
        "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
        "/ProjectProperties/MakefileSettings/preprocessorIncludes/listItem/text()",
        actcfg);
    XPathObject preincludesObj(run_xpath(xpathCtx, preincludesXpath));
    Free(preincludesXpath);

    xmlNodeSetPtr nodes = preincludesObj->nodesetval;

    if (nodes) for (int i = 0; i < nodes->nodeNr; ++i) {
      char* content = (char*)preincludesObj->nodesetval->nodeTab[i]->content;

      // add includes to the end of list
      if (prep_includes) {
        // go to last element
        struct string_list* last_elem = prep_includes;
        while (last_elem->next) last_elem = last_elem->next;
        // add string to last element if empty or create new last element and add it to that
        if (last_elem->str) {
          last_elem->next = (struct string_list*)Malloc(sizeof(struct string_list));
          last_elem = last_elem->next;
          last_elem->next = NULL;
        }
        replacechar(&content);
        last_elem->str = content;
      }
    }
  }
  {
    //preprocessorDefines
    char *predefinesXpath = mprintf(
        "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
        "/ProjectProperties/MakefileSettings/preprocessorDefines/listItem/text()",
        actcfg);
    XPathObject predefinesObj(run_xpath(xpathCtx, predefinesXpath));
    Free(predefinesXpath);

    xmlNodeSetPtr nodes = predefinesObj->nodesetval;

    if (nodes) for (int i = 0; i < nodes->nodeNr; ++i) {
      const char* content = (const char*)predefinesObj->nodesetval->nodeTab[i]->content;

      // add includes to the end of list
      if (prep_defines) {
        // go to last element
        struct string_list* last_elem = prep_defines;
        while (last_elem->next) last_elem = last_elem->next;
        // add string to last element if empty or create new last element and add it to that
        if (last_elem->str) {
          last_elem->next = (struct string_list*)Malloc(sizeof(struct string_list));
          last_elem = last_elem->next;
          last_elem->next = NULL;
        }
        last_elem->str = mcopystr(content);
      }
    }
  }
  {
    //preprocessorUnDefines
    char *preUndefinesXpath = mprintf(
        "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
        "/ProjectProperties/MakefileSettings/preprocessorUndefines/listItem/text()",
        actcfg);
    XPathObject preUndefinesObj(run_xpath(xpathCtx, preUndefinesXpath));
    Free(preUndefinesXpath);

    xmlNodeSetPtr nodes = preUndefinesObj->nodesetval;

    if (nodes) for (int i = 0; i < nodes->nodeNr; ++i) {
      const char* content = (const char*)preUndefinesObj->nodesetval->nodeTab[i]->content;

      // add includes to the end of list
      if (prep_undefines) {
        // go to last element
        struct string_list* last_elem = prep_undefines;
        while (last_elem->next) last_elem = last_elem->next;
        // add string to last element if empty or create new last element and add it to that
        if (last_elem->str) {
          last_elem->next = (struct string_list*)Malloc(sizeof(struct string_list));
          last_elem = last_elem->next;
          last_elem->next = NULL;
        }
        last_elem->str = mcopystr(content);
      }
    }
  }
  {
    char *cxxCompilerXpath = mprintf(
            "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
            "/ProjectProperties/MakefileSettings/CxxCompiler/text()",
            actcfg);
    XPathObject cxxCompilerObj(run_xpath(xpathCtx, cxxCompilerXpath));
    Free(cxxCompilerXpath);
    xmlNodeSetPtr nodes = cxxCompilerObj->nodesetval;
    if (nodes) {
      *cxxcompiler = mcopystr((const char*)cxxCompilerObj->nodesetval->nodeTab[0]->content);
    }
  }
  {
    char *optLevelXpath = mprintf(
        "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
        "/ProjectProperties/MakefileSettings/optimizationLevel/text()",
        actcfg);
    XPathObject optLevelObj(run_xpath(xpathCtx, optLevelXpath));
    Free(optLevelXpath);
    xmlNodeSetPtr nodes = optLevelObj->nodesetval;
    if (nodes) {
      *optlevel = mcopystr((const char*)optLevelObj->nodesetval->nodeTab[0]->content);
    }
  }
  {
    char *optFlagsXpath = mprintf(
        "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
        "/ProjectProperties/MakefileSettings/otherOptimizationFlags/text()",
        actcfg);
    XPathObject optFlagsObj(run_xpath(xpathCtx, optFlagsXpath));
    Free(optFlagsXpath);
    xmlNodeSetPtr nodes = optFlagsObj->nodesetval;
    if (nodes) {
      *optflags = mcopystr((const char*)optFlagsObj->nodesetval->nodeTab[0]->content);
    }
  }
  {
    //SolarisSpecificLibraries
    char *solspeclibXpath = mprintf(
        "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
        "/ProjectProperties/MakefileSettings/SolarisSpecificLibraries/listItem/text()",
        actcfg);
    XPathObject solspeclibObj(run_xpath(xpathCtx, solspeclibXpath));
    Free(solspeclibXpath);

    xmlNodeSetPtr nodes = solspeclibObj->nodesetval;

    if (nodes) for (int i = 0; i < nodes->nodeNr; ++i) {
      char* content = (char*)solspeclibObj->nodesetval->nodeTab[i]->content;

      // add includes to the end of list
      if (solspeclibs) {
        // go to last element
        struct string_list* last_elem =solspeclibs;
        while (last_elem->next) last_elem = last_elem->next;
        // add string to last element if empty or create new last element and add it to that
        if (last_elem->str) {
          last_elem->next = (struct string_list*)Malloc(sizeof(struct string_list));
          last_elem = last_elem->next;
          last_elem->next = NULL;
        }
        replacechar(&content);
        last_elem->str = content;
      }
    }
  }
  {
    //Solaris8SpecificLibraries
    char *sol8speclibXpath = mprintf(
        "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
        "/ProjectProperties/MakefileSettings/Solaris8SpecificLibraries/listItem/text()",
        actcfg);
    XPathObject sol8speclibObj(run_xpath(xpathCtx, sol8speclibXpath));
    Free(sol8speclibXpath);

    xmlNodeSetPtr nodes = sol8speclibObj->nodesetval;

    if (nodes) for (int i = 0; i < nodes->nodeNr; ++i) {
      char* content = (char*)sol8speclibObj->nodesetval->nodeTab[i]->content;

      // add includes to the end of list
      if (sol8speclibs) {
        // go to last element
        struct string_list* last_elem = sol8speclibs;
        while (last_elem->next) last_elem = last_elem->next;
        // add string to last element if empty or create new last element and add it to that
        if (last_elem->str) {
          last_elem->next = (struct string_list*)Malloc(sizeof(struct string_list));
          last_elem = last_elem->next;
          last_elem->next = NULL;
        }
        replacechar(&content);
        last_elem->str = content;
      }
    }
  }
  {
    //LinuxSpecificLibraries
    char *linuxspeclibXpath = mprintf(
        "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
        "/ProjectProperties/MakefileSettings/LinuxSpecificLibraries/listItem/text()",
        actcfg);
    XPathObject linuxspeclibObj(run_xpath(xpathCtx, linuxspeclibXpath));
    Free(linuxspeclibXpath);

    xmlNodeSetPtr nodes = linuxspeclibObj->nodesetval;

    if (nodes) for (int i = 0; i < nodes->nodeNr; ++i) {
      char* content = (char*)linuxspeclibObj->nodesetval->nodeTab[i]->content;

      // add includes to the end of list
      if (linuxspeclibs) {
        // go to last element
        struct string_list* last_elem = linuxspeclibs;
        while (last_elem->next) last_elem = last_elem->next;
        // add string to last element if empty or create new last element and add it to that
        if (last_elem->str) {
          last_elem->next = (struct string_list*)Malloc(sizeof(struct string_list));
          last_elem = last_elem->next;
          last_elem->next = NULL;
        }
        replacechar(&content);
        last_elem->str = content;
      }
    }
  }
  {
    //FreeBSDSpecificLibraries
    char *freebsdspeclibXpath = mprintf(
        "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
        "/ProjectProperties/MakefileSettings/FreeBSDSpecificLibraries/listItem/text()",
        actcfg);
    XPathObject freebsdspeclibObj(run_xpath(xpathCtx, freebsdspeclibXpath));
    Free(freebsdspeclibXpath);

    xmlNodeSetPtr nodes = freebsdspeclibObj->nodesetval;

    if (nodes) for (int i = 0; i < nodes->nodeNr; ++i) {
      char* content = (char*)freebsdspeclibObj->nodesetval->nodeTab[i]->content;

      // add includes to the end of list
      if (freebsdspeclibs) {
        // go to last element
        struct string_list* last_elem = freebsdspeclibs;
        while (last_elem->next) last_elem = last_elem->next;
        // add string to last element if empty or create new last element and add it to that
        if (last_elem->str) {
          last_elem->next = (struct string_list*)Malloc(sizeof(struct string_list));
          last_elem = last_elem->next;
          last_elem->next = NULL;
        }
        replacechar(&content);
        last_elem->str = content;
      }
    }
  }
  {
    //Win32SpecificLibraries
    char *win32speclibXpath = mprintf(
        "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
        "/ProjectProperties/MakefileSettings/Win32SpecificLibraries/listItem/text()",
        actcfg);
    XPathObject win32speclibObj(run_xpath(xpathCtx, win32speclibXpath));
    Free(win32speclibXpath);

    xmlNodeSetPtr nodes = win32speclibObj->nodesetval;

    if (nodes) for (int i = 0; i < nodes->nodeNr; ++i) {
      char* content = (char*)win32speclibObj->nodesetval->nodeTab[i]->content;

      // add includes to the end of list
      if (win32speclibs) {
        // go to last element
        struct string_list* last_elem = win32speclibs;
        while (last_elem->next) last_elem = last_elem->next;
        // add string to last element if empty or create new last element and add it to that
        if (last_elem->str) {
          last_elem->next = (struct string_list*)Malloc(sizeof(struct string_list));
          last_elem = last_elem->next;
          last_elem->next = NULL;
        }
        replacechar(&content);
        last_elem->str = content;
      }

    }
  }
  {
    //TTCN3preprocessor
    char *ttcn3preproc = mprintf(
        "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
        "/ProjectProperties/MakefileSettings/TTCN3preprocessor/text()",
        actcfg);
    XPathObject ttcn3preprocObj(run_xpath(xpathCtx, ttcn3preproc));
    Free(ttcn3preproc);
    xmlNodeSetPtr nodes = ttcn3preprocObj->nodesetval;
    if (nodes) {
      *ttcn3prep = mcopystr((const char*)ttcn3preprocObj->nodesetval->nodeTab[0]->content);
    }
  }
  {
    // additionalObjects
    char *additionalObjectsXpath = mprintf(
        "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
        "/ProjectProperties/MakefileSettings/additionalObjects/listItem/text()",
        actcfg);
    XPathObject additionalObjectsObj(run_xpath(xpathCtx, additionalObjectsXpath));
    Free(additionalObjectsXpath);

    xmlNodeSetPtr nodes = additionalObjectsObj->nodesetval;

    if (nodes) for (int i = 0; i < nodes->nodeNr; ++i) {
      char* content = (char*)additionalObjectsObj->nodesetval->nodeTab[i]->content;

      // add to the end of list
      if (additionalObjects) {
        // go to last element
        struct string_list* last_elem = additionalObjects;
        while (last_elem->next) last_elem = last_elem->next;
        // add string to last element if empty or create new last element and add it to that
        if (last_elem->str) {
          last_elem->next = (struct string_list*)Malloc(sizeof(struct string_list));
          last_elem = last_elem->next;
          last_elem->next = NULL;
        }
        replacechar(&content);
        last_elem->str = content;
      }
    }
  }
  {
    //The project name needed the hierarchical projects
    char* prjNameStr = 0;
    char *prjNameStrXpath = mprintf("/TITAN_Project_File_Information/ProjectName/text()");
    XPathObject  prjName(run_xpath(xpathCtx, prjNameStrXpath));
    if (prjName->nodesetval && prjName->nodesetval->nodeNr == 1) {
      prjNameStr = (char*)prjName->nodesetval->nodeTab[0]->content;
    }
    Free(prjNameStrXpath);
    append_to_library_list (prjNameStr, xpathCtx, actcfg);

    //linkerLibraries
    char *linkerlibsXpath = mprintf(
        "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
        "/ProjectProperties/MakefileSettings/linkerLibraries/listItem/text()",
        actcfg);
    XPathObject linkerlibsObj(run_xpath(xpathCtx, linkerlibsXpath));
    Free(linkerlibsXpath);

    xmlNodeSetPtr nodes = linkerlibsObj->nodesetval;

    if (nodes) for (int i = 0; i < nodes->nodeNr; ++i) {
      char* content = (char*)linkerlibsObj->nodesetval->nodeTab[i]->content;

      // add includes to the end of list
      if (linkerlibs) {
        // go to last element
        struct string_list* last_elem = linkerlibs;
        while (last_elem->next) last_elem = last_elem->next;
        // add string to last element if empty or create new last element and add it to that
        if (last_elem->str) {
          last_elem->next = (struct string_list*)Malloc(sizeof(struct string_list));
          last_elem = last_elem->next;
          last_elem->next = NULL;
        }
        replacechar(&content);
        last_elem->str = content;

        ProjectDescriptor* projDescr = projGenHelper.getTargetOfProject(*p_project_name);
        if (projDescr) projDescr->addToLinkerLibs(last_elem->str);
      }
    }
  }
  {
    //linkerLibrarySearchPath
    char *linkerlibsearchXpath = mprintf(
        "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
        "/ProjectProperties/MakefileSettings/linkerLibrarySearchPath/listItem/text()",
        actcfg);
    XPathObject linkerlibsearchObj(run_xpath(xpathCtx, linkerlibsearchXpath));
    Free(linkerlibsearchXpath);

    xmlNodeSetPtr nodes = linkerlibsearchObj->nodesetval;

    if (nodes) for (int i = 0; i < nodes->nodeNr; ++i) {
      char* content = (char*)linkerlibsearchObj->nodesetval->nodeTab[i]->content;

      // add includes to the end of list
      if (linkerlibsearchp) {
        // go to last element
        struct string_list* last_elem = linkerlibsearchp;
        while (last_elem->next) last_elem = last_elem->next;
        // add string to last element if empty or create new last element and add it to that
        if (last_elem->str) {
          last_elem->next = (struct string_list*)Malloc(sizeof(struct string_list));
          last_elem = last_elem->next;
          last_elem->next = NULL;
        }
        replacechar(&content);
        last_elem->str = content;

        ProjectDescriptor* projDescr = projGenHelper.getTargetOfProject(*p_project_name);
        if (projDescr) projDescr->addToLibSearchPaths(last_elem->str);
      }
    }
  }

  if (generatorCommandOutput && local_argc != 0) { // only in case of top-level invocation
    char* generatorCommandXpath = mprintf(
      "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
      "/ProjectProperties/MakefileSettings/ProjectSpecificRulesGenerator/GeneratorCommand/text()",
      actcfg);
    XPathObject generatorCommandObj(run_xpath(xpathCtx, generatorCommandXpath));
    Free(generatorCommandXpath);
    autostring generatorCommand;
    if (generatorCommandObj->nodesetval && generatorCommandObj->nodesetval->nodeNr > 0) {
      generatorCommand = mcopystr((const char*)generatorCommandObj->nodesetval->nodeTab[0]->content);
      // run the command and capture the output
      printf("Executing generator command `%s' specified in `%s'...\n", (const char*)generatorCommand, (const char*)abs_tpd_name);
      FILE* gc_fp = popen(generatorCommand, "r");
      if (!gc_fp) {
        ERROR("Could not execute command `%s'", (const char*)generatorCommand);
      } else {
        char buff[1024];
        while (fgets(buff, sizeof(buff), gc_fp)!=NULL) {
          *generatorCommandOutput = mputstr(*generatorCommandOutput, buff);
        }
        pclose(gc_fp);
      }
    }
  }

  if (target_placement_list && local_argc != 0) { // only in case of top-level invocation
    char* targetPlacementXpath = mprintf(
      "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
      "/ProjectProperties/MakefileSettings/ProjectSpecificRulesGenerator/Targets/Target/attribute::*",
      actcfg);
    XPathObject targetPlacementObj(run_xpath(xpathCtx, targetPlacementXpath));
    Free(targetPlacementXpath);
    xmlNodeSetPtr nodes = targetPlacementObj->nodesetval;
    const char* targetName = NULL;
    const char* targetPlacement = NULL;
    if (nodes) for (int i = 0; i < nodes->nodeNr; ++i) {
      if (!strcmp((const char*)nodes->nodeTab[i]->name, "name")) {
        targetName = (const char*)nodes->nodeTab[i]->children->content;
      }
      else if (!strcmp((const char*)nodes->nodeTab[i]->name,"placement")) {
        targetPlacement = (const char*)nodes->nodeTab[i]->children->content;
      }
      if (targetName && targetPlacement) { // collected both
        if (target_placement_list) {
          // go to last element
          struct string2_list* last_elem = target_placement_list;
          while (last_elem->next) last_elem = last_elem->next;
          // add strings to last element if empty or create new last element and add it to that
          if (last_elem->str1) {
            last_elem->next = (struct string2_list*)Malloc(sizeof(struct string2_list));
            last_elem = last_elem->next;
            last_elem->next = NULL;
          }
          last_elem->str1 = mcopystr(targetName);
          last_elem->str2 = mcopystr(targetPlacement);
        }
        targetName = targetPlacement = NULL; // forget both
      }
    }
  }
  
  {
    // profiler file name
    char* profilerXpath = mprintf(
      "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
      "/ProjectProperties/MakefileSettings/profiledFileList", actcfg);
    XPathObject profiledFilesNameObj(run_xpath(xpathCtx, profilerXpath));
    Free(profilerXpath);
    xmlNodeSetPtr nodes = profiledFilesNameObj->nodesetval;
    if (nodes && nodes->nodeNr > 0) {
      const char *uri = 0, *path = 0, *raw = 0;
      for (xmlAttrPtr attr = nodes->nodeTab[0]->properties; attr; attr = attr->next) {
        if (!strcmp((const char*)attr->name, "projectRelativePath")) {
          path = (const char*)attr->children->content;
        }
        else if (!strcmp((const char*)attr->name, "relativeURI")) {
          uri = (const char*)attr->children->content;
        }
        else if (!strcmp((const char*)attr->name, "rawURI")) {
          raw = (const char*)attr->children->content;
        }
        else {
          WARNING("Unknown attribute %s", (const char*)nodes->nodeTab[0]->name);
        }
      } // next attribute
      
      if (path == NULL) {
        ERROR("A profiledFileList must have a projectRelativePath");
      }
      else {
        if (uri == NULL && raw == NULL) {
          ERROR("A profiledFileList must have either relativeURI or rawURI");
        }
        else {
          cstring cpath(path);
          if (files.has_key(cpath)) {
            ERROR("profiledFileLists and FileResources must be unique!");
          }
          else {
            // add the file to the end of the list
            struct string_list* new_item = NULL;
            if (*profiled_file_list == NULL) {
              *profiled_file_list = (struct string_list*)Malloc(sizeof(struct string_list));
              new_item = *profiled_file_list;
            }
            else {
              new_item = *profiled_file_list;
              while(new_item->next != NULL) {
                new_item = new_item->next;
              }
              new_item->next = (struct string_list*)Malloc(sizeof(struct string_list));
              new_item = new_item->next;
            }
            new_item->str = mcopystr(path);
            new_item->next = NULL;
            
            // add the file to the map of files to be copied to the working directory
            char *ruri = uri ? mcopystr(uri) : cook(raw, path_vars);
            files.add(cpath, ruri);
          }
        }
      }
    }
  }
  } // If get_config_mode
// collect the required configurations
  
  struct config_struct* config_elem = all_configs;
  {
    if (required_configs && get_config_mode) {

      char* cfgConfigsXpath(mprintf(
        "/TITAN_Project_File_Information/Configurations/Configuration"));
      XPathObject cfgConfigsObjects(run_xpath(xpathCtx, cfgConfigsXpath));
      Free(cfgConfigsXpath);
      xmlNodeSetPtr configs = cfgConfigsObjects->nodesetval;
      if (configs) {
        // Go through configurations
        for (int i = 0; i < configs->nodeNr; ++i) {
          xmlAttrPtr currentNode = configs->nodeTab[i]->properties;
          const char* configName = NULL;
          
          for (xmlAttrPtr attr = currentNode; attr; attr = attr->next) {
            if (!strcmp((const char*)attr->name, "name")) {
              configName = (const char*)attr->children->content;
            }
          }
          char* cfgReqsXpath(mprintf(
            "/TITAN_Project_File_Information/Configurations/Configuration[@name='%s']"
            "/ProjectProperties/ConfigurationRequirements/configurationRequirement"
            , configName));

          while(config_elem->next != NULL) {
            config_elem = config_elem->next;
          }
          config_elem->project_name = mcopystr(*p_project_name);
          config_elem->project_conf = mcopystr(configName);
          config_elem->is_active = strcmp(configName, actcfg) == 0;
          struct config_struct* next_elem = (struct config_struct*)Malloc(sizeof(struct config_struct));
          config_struct_init(next_elem);
          config_elem->next = next_elem;

          XPathObject reqcfgObjects(run_xpath(xpathCtx, cfgReqsXpath));
          Free (cfgReqsXpath);
          xmlNodeSetPtr reqConfigs = reqcfgObjects->nodesetval;
          // Go through the required configurations of configurations
          if (reqConfigs) for (int j = 0; j < reqConfigs->nodeNr; ++j) {
            xmlNodePtr curNodePtr = reqConfigs->nodeTab[j]->children;
            const char* projectName = NULL;
            const char* reqConfig = NULL;
            while(curNodePtr) {
              if (!strcmp((const char*)curNodePtr->name, "projectName")) {
                projectName = (const char*)curNodePtr->children->content;
              }
              if (!strcmp((const char*)curNodePtr->name, "rerquiredConfiguration") || // backward compatibility
                  !strcmp((const char*)curNodePtr->name, "requiredConfiguration")) {
                  reqConfig = (const char*)curNodePtr->children->content;
              }
              curNodePtr = curNodePtr->next;
            }
            
            // Check if the required configuration is already inserted.
            struct string2_list* last_req = config_elem->requirements;
            boolean already_in = FALSE;
            while (last_req && last_req->str1 != NULL && last_req->str2 != NULL && already_in == FALSE) {
              if (strcmp(last_req->str1, projectName) == 0 && strcmp(last_req->str2, reqConfig) == 0) {
                already_in = TRUE;
                break;
              }
              last_req = last_req->next;
            }
            // If not, insert into the requirements of the configuration
            if (already_in == FALSE) {
              last_req->str1 = mcopystr(projectName);
              last_req->str2 = mcopystr(reqConfig);
              struct string2_list* next_req = (struct string2_list*)Malloc(sizeof(struct string2_list));
              next_req->str1 = NULL;
              next_req->str2 = NULL;
              next_req->next = NULL;
              last_req->next = next_req;
              last_req = next_req;
            }

          } // ReqConfigs
        } // Configs
      } // If configs
    }
  }
  // Referenced projects
  {
    XPathObject subprojects(run_xpath(xpathCtx,
      "/TITAN_Project_File_Information/ReferencedProjects/ReferencedProject"));
    xmlNodeSetPtr nodes = subprojects->nodesetval;
    //Go through ReferencedProjects
    if (nodes) for (int i = 0; i < nodes->nodeNr; ++i) {
      const char *name = NULL, *projectLocationURI = NULL;
      char *tpdName_loc = NULL;
      // FIXME: this assumes every ReferencedProject has name and URI.
      // This is not necessarily so if the referenced project was closed
      // when the project was exported to TPD.
      // Luckily, the name from the closed project will be overwritten
      // by the name from the next ReferencedProject. However, if some pervert
      // changes the next ReferencedProject to have the projectLocationURI
      // as the first attribute, it will be joined to the name
      // of the previous, closed, ReferencedProject.
      
      //Go through attributes
      for (xmlAttrPtr attr = nodes->nodeTab[i]->properties; attr; attr = attr->next) {
        if (!strcmp((const char*)attr->name, "name")) {
          name = (const char*)attr->children->content;
        }
        else if (!strcmp((const char*)attr->name,"projectLocationURI")) {
          projectLocationURI = (const char*)attr->children->content;
        }
        else if (!strcmp((const char*)attr->name, "tpdName")) {
          //Allocate memory
          tpdName_loc = mcopystr((char*)attr->children->content);
        }
      }
      //We don't want to orerride an absolute location with -I, tpdName remains NULL
      boolean not_abs_path = projectLocationURI &&
#if defined WIN32 && defined MINGW
	/* On native Windows the absolute path name shall begin with
	 * a drive letter, colon and backslash */
	(((projectLocationURI[0] < 'A' || projectLocationURI[0] > 'Z') &&
	  (projectLocationURI[0] < 'a' || projectLocationURI[0] > 'z')) ||
	 projectLocationURI[1] != ':' || projectLocationURI[2] != '\\');
#else
	/* On UNIX-like systems the absolute path name shall begin with
	 * a slash */
	projectLocationURI[0] != '/';
#endif
      if (!tpdName_loc && not_abs_path) {
        //Allocate memory: +5 because .tpd + closing 0
        tpdName_loc = (char*)Malloc((strlen(name) + 5) * sizeof(char));
        //Default name: name + .tpd
        strcpy(tpdName_loc, name);
        strcat(tpdName_loc, ".tpd");
      }else if (!not_abs_path) {
        Free(tpdName_loc);
        tpdName_loc = NULL;
      }
      
      if (name && projectLocationURI) { // collected both
        // see if there is a specified configuration for the project

        ProjectDescriptor* projDesc = projGenHelper.getTargetOfProject(*p_project_name);
        if (projDesc) projDesc->addToReferencedProjects(name);
        
        const char *my_actcfg = NULL;
        int my_argc = 0;
        char *my_args[] = { NULL };
        char **my_argv = my_args + 0;
        boolean my_free_argv = FALSE;
        int my_optind = 0;
        boolean my_gflag = *p_gflag, my_aflag = *p_aflag, my_cflag = *p_cflag, // pass down
          my_Rflag = *p_Rflag, my_Pflag = *p_Pflag, my_Zflag = *p_Zflag, my_Hflag = *p_Hflag,
          my_sflag =  0, my_Lflag =  0, my_lflag =  0, my_mflag =  0,
          my_quflag = 0, my_dsflag = 0, my_dbflag = 0, my_drflag = 0,
          my_dtflag = 0, my_dxflag = 0, my_djflag = 0, my_fxflag = 0, my_doflag = 0, 
          my_gfflag = 0, my_lnflag = 0, my_isflag = 0, my_asflag = 0, 
          my_swflag = 0, my_Yflag = 0, my_Mflag = *p_Mflag, my_Eflag = 0, my_nflag = *p_nflag,
          my_Nflag = 0,
          my_diflag = *p_diflag;

        char *my_ets = NULL;
        char *my_proj_name = NULL;
        char *my_csmode = *p_csmode;
        autostring abs_projectLocationURI;
        if (not_abs_path) {
          abs_projectLocationURI = compose_path_name(abs_tpd_dir, projectLocationURI);
        } else {
          //If absolute directory, then just copy the URI
          abs_projectLocationURI = mcopystr(projectLocationURI);
        }

        char* sub_proj_abs_work_dir = NULL;
        
        //Insert children of the project
        if (get_config_mode && config_elem->project_name != NULL) {
          struct config_struct* tmp = all_configs;
          boolean already_in = FALSE;
          while (tmp && tmp->project_name != NULL && already_in == FALSE) {
            if (strcmp(tmp->project_name, config_elem->project_name) == 0) {
              struct string_list* last_child = tmp->children;
              while(last_child && last_child->str != NULL) {
                if (strcmp(last_child->str, name) == 0) {
                  already_in = TRUE;
                  break;
                }
                last_child = last_child->next;
              }
              if (already_in == FALSE) {
                last_child->str = mcopystr(name);
                struct string_list* next_child = (struct string_list*)Malloc(sizeof(string_list));
                next_child->str = NULL;
                next_child->next = NULL;
                last_child->next = next_child;
                last_child = next_child;
                //break; needed???
              }
            }
            tmp = tmp->next;
          }
        }
      
        const char* abs_projectLocationURIchar = abs_projectLocationURI.extract();
        tpd_result success = process_tpd_internal(&abs_projectLocationURIchar, tpdName_loc,
          my_actcfg, file_list_path, &my_argc, &my_argv, &my_free_argv, &my_optind, &my_ets, &my_proj_name,
          &my_gflag, &my_sflag, &my_cflag, &my_aflag, preprocess, &my_Rflag, &my_lflag,
          &my_mflag, &my_Pflag, &my_Lflag, recursive, force_overwrite, gen_only_top_level, NULL, &sub_proj_abs_work_dir,
          sub_project_dirs, program_name, prj_graph_fp, create_symlink_list, ttcn3_prep_includes, ttcn3_prep_defines, ttcn3_prep_undefines, 
          prep_includes, prep_defines, prep_undefines, &my_csmode,
          &my_quflag, &my_dsflag, cxxcompiler, optlevel, optflags, &my_dbflag, &my_drflag,
          &my_dtflag, &my_dxflag, &my_djflag, &my_fxflag, &my_doflag,
          &my_gfflag, &my_lnflag, &my_isflag, &my_asflag, &my_swflag, &my_Yflag, &my_Mflag, &my_Eflag, &my_nflag, &my_Nflag, &my_diflag,
          solspeclibs, sol8speclibs, linuxspeclibs, freebsdspeclibs, win32speclibs,
          ttcn3prep, linkerlibs, additionalObjects, linkerlibsearchp, Vflag, FALSE, &my_Zflag, 
          &my_Hflag, NULL, NULL, prefix_workdir, run_command_list, seen_tpd_files, required_configs, profiled_file_list,
          search_paths, n_search_paths, makefileScript, all_configs);
        
        autostring sub_proj_abs_work_dir_as(sub_proj_abs_work_dir); // ?!
        abs_projectLocationURI = abs_projectLocationURIchar;
        if (get_config_mode) {
          if (!projGenHelper.insertAndCheckProjectName((const char*)abs_projectLocationURI, name)) {
            ERROR("In project `%s', the referenced project `%s's name attribute should be the same as the referenced project's name.",
              *p_tpd_name, (const char*)abs_projectLocationURI);
          }
        }
        
        if (success == TPD_SUCCESS && !get_config_mode) {
          my_actcfg = get_act_config(required_configs, my_proj_name);
          if (recursive) { // call ttcn3_makefilegen on referenced project's tpd file
            // -r is not needed any more because top level process traverses all projects recursively
            expstring_t command = mprintf("%s -cVD", program_name);
            if (force_overwrite) command = mputc(command, 'f');
            if (prefix_workdir) command = mputc(command, 'W');
            if (*p_gflag) command = mputc(command, 'g');
            if (*p_sflag) command = mputc(command, 's');
            if (*p_aflag) command = mputc(command, 'a');
            if (*p_Rflag) command = mputc(command, 'R');
            if (*p_lflag) command = mputc(command, 'l');
            if (*p_mflag) command = mputc(command, 'm');
            if (*p_Zflag) command = mputc(command, 'Z');
            if (*p_Hflag) command = mputc(command, 'H');
            command = mputstr(command, " -t ");
            command = mputstr(command, (const char*)abs_projectLocationURI);
            if (my_actcfg) {
              command = mputstr(command, " -b ");
              command = mputstr(command, my_actcfg);
            }

            autostring sub_tpd_dir(get_dir_from_path((const char*)abs_projectLocationURI));
            const char * sub_proj_effective_work_dir = sub_proj_abs_work_dir ? sub_proj_abs_work_dir : (const char*)sub_tpd_dir;
            if (!gen_only_top_level) {
              if (run_command_list) {
                // go to last element
                struct string2_list* last_elem = run_command_list;
                while (last_elem->next) last_elem = last_elem->next;
                // add strings to last element if empty or create new last element and add it to that
                if (last_elem->str1 || last_elem->str2) {
                  last_elem->next = (struct string2_list*)Malloc(sizeof(struct string2_list));
                  last_elem = last_elem->next;
                  last_elem->next = NULL;
                }
                last_elem->str1 = mcopystr(sub_proj_effective_work_dir);
                last_elem->str2 = command;
              } else {
                ERROR("Internal error: cannot add command to list");
              }
            } else {
              Free(command);
            }
            // add working dir to the end of list
            if (sub_project_dirs) {
              // go to last element
              struct string_list* last_elem = sub_project_dirs;
              while (last_elem->next) last_elem = last_elem->next;
              // add string to last element if empty or create new last element and add it to that
              if (last_elem->str) {
                last_elem->next = (struct string_list*)Malloc(sizeof(struct string_list));
                last_elem = last_elem->next;
                last_elem->next = NULL;
              }
              autostring cwd_as(get_working_dir());
              last_elem->str = (*p_aflag) ? mcopystr(sub_proj_effective_work_dir) : get_relative_dir(sub_proj_effective_work_dir, (const char*)cwd_as);
            }
          }

          for (int z = 0; z < my_argc; ++z) {
            if (*p_cflag) {
              // central storage, keep in separate container
              base_files.add(my_argv[z]); // string was allocated with new
            }
            else {
              const cstring tmp(my_argv[z]);
              if (!files.has_key(tmp)){
                files.add(tmp, my_argv[z]);
              } else if (my_free_argv) {
                Free(my_argv[z]);
              }
            }
          }

          if (my_free_argv) {
            Free(my_argv); // free the array; we keep the pointers
          }
          Free(my_ets);
        }
        else {
          if (my_free_argv) {
            for (int z = 0; z < my_argc; ++z) {
              Free(my_argv[z]);
            }
            Free(my_argv);
          }
          if (success == TPD_FAILED) {
            ERROR("Failed to process %s", (const char*)abs_projectLocationURI);
            result = TPD_FAILED;
          }
          // else TPD_SKIPPED, keep quiet
        }
        Free(my_proj_name);
        Free(tpdName_loc);
        name = projectLocationURI = tpdName_loc = NULL; // forget all
      }
    } // next referenced project
  }

  if (output_file) {
    if (get_path_status(output_file) == PS_DIRECTORY) {
      // points to existing dir; use as-is
    }
    else { // we assume it points to a file: not our problem
      output_file = NULL;
    }
  }

  // (argc - optind) is the number of non-option arguments (assumed to be files)
  // given on the command line.
  int new_argc = local_argc - local_optind + files.size() + base_files.size();
  char ** new_argv = (char**)Malloc(sizeof(char*) * new_argc);

  int n = 0;

  // First, copy the filenames gathered from the TPD
  //
  // We symlink the files into the working directory
  // and pass only the filename to the makefile generator
  for (int nf = files.size(); n < nf; ++n) {
    const char *fn = files.get_nth_elem(n); // relativeURI to the TPD location
    autostring  dir_n (get_dir_from_path (fn));
    autostring  file_n(get_file_from_path(fn));
    autostring  rel_n (get_absolute_dir(dir_n, abs_tpd_dir, TRUE));
    autostring  abs_n (compose_path_name(rel_n, file_n));

    if (local_argc == 0) {
      // We are being invoked recursively, for a referenced TPD.
      // Do not symlink; just return absolute paths to the files.
      if (*fn == '/') {
        if (*p_cflag) {
          // compose with workdir
          new_argv[n] = compose_path_name(abs_workdir, file_n);
        } else {
          // it's an absolute path, copy verbatim
          new_argv[n] = mcopystr(fn); // fn will be destroyed, pass a copy
        }
      }
      else { // relative path
        if (*p_cflag) {
          // compose with workdir
          new_argv[n] = compose_path_name(abs_workdir, file_n);
          // Do not call file_n.extract() : the composed path will be returned,
          // its component will need to be deallocated here.
        }
        else {
          // compose with tpd dir
          new_argv[n] = const_cast<char*>(abs_n.extract());
        }
      }
    }
    else { // we are processing the top-level TPD
#ifndef MINGW
      if (!*p_Pflag) {
        int fd = open(abs_n, O_RDONLY);
        if (fd >= 0) { // successfully opened
          close(fd);
          if (output_file) {
            file_n = compose_path_name(output_file, file_n);
          }
//TODO ! compose with output_file
          // save into list: add symlink data to the end of list
          if (create_symlink_list) {
            // go to last element
            struct string2_list* last_elem = create_symlink_list;
            while (last_elem->next) last_elem = last_elem->next;
            // add strings to last element if empty or create new last element and add it to that
            if (last_elem->str1) {
              last_elem->next = (struct string2_list*)Malloc(sizeof(struct string2_list));
              last_elem = last_elem->next;
              last_elem->next = NULL;
            }
            last_elem->str1 = mcopystr(abs_n);
            last_elem->str2 = mcopystr(file_n);
          } 
        }
        else {
          ERROR("%s does not exist", (const char*)abs_n);
        }
      }
#endif
      if (*p_Pflag) {
        if (*p_aflag) {
          puts((const char *)abs_n);
        } else {
          autostring dir_part(get_dir_from_path(abs_n));
          autostring file_part(get_file_from_path(abs_n));
          autostring rel_dir_part(get_relative_dir((const char *)dir_part, file_list_path ? file_list_path : (const char *)abs_tpd_dir));
          autostring rel_dir_file_part(compose_path_name((const char *)rel_dir_part, (const char *)file_part));
          puts((const char *)rel_dir_file_part);
        }
      }
      new_argv[n] = const_cast<char *>(file_n.extract());
    }
  }
  // Print the TPD too.
  if (*p_Pflag) {
    autostring dir_part(get_dir_from_path(*p_tpd_name));
    autostring file_part(get_file_from_path(*p_tpd_name));
    if (*p_aflag) {
      puts((const char *)abs_tpd_name);
    } else {
      autostring rel_dir_part(get_relative_dir(dir_part, file_list_path ? file_list_path : abs_tpd_dir));
      autostring rel_dir_file_part(compose_path_name(rel_dir_part, file_part));
      const char *rel_tpd_name = (const char *)rel_dir_file_part;
      puts(rel_tpd_name);
    }
  }

  // base_files from referenced projects
  for (size_t bf = 0, bs = base_files.size(); bf < bs; ++bf, ++n) {
    new_argv[n] = base_files[bf];
  }
  base_files.clear(); // string ownership transfered

  // Then, copy the filenames from the command line.
  for (int a = *p_optind; a < *p_argc; ++a, ++n) {
    // String may be from main's argv; copy to the heap.
    new_argv[n] = mcopystr((*p_argv)[a]);
  }

  if (local_argc > 0) { // it is the outermost call
    clear_seen_tpd_files(seen_tpd_files);
  }
  // replace argv only if not config mode
  if (!get_config_mode) {
    if (*p_free_argv) {
      for (int i = 0; i < *p_argc; ++i) {
        Free((*p_argv)[i]);
      }
      Free(*p_argv);
    }
    else {
      *p_free_argv = TRUE;
    }
    *p_argv = new_argv;
    *p_argc = new_argc;
    *p_optind = 0;
  } else {
    for (int i = 0; i < new_argc; ++i) {
      Free(new_argv[i]);
    }
    Free(new_argv);
  }

  // finally...
  for (size_t i = 0, e = files.size(); i < e; ++i) {
    Free(const_cast<char*>(files.get_nth_elem(i)));
  }
  files.clear();

  for (size_t i = 0, e = folders.size(); i < e; ++i) {
    Free(const_cast<char*>(folders.get_nth_elem(i)));
  }
  folders.clear();

  excluded_files.clear();
  excluded_folders.clear();
  path_vars.clear();

  xmlCleanupParser();
  // ifdef debug
  xmlMemoryDump();
  return result;
}
