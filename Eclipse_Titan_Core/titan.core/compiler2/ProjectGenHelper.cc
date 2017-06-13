///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2000-2017 Ericsson Telecom AB
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the Eclipse Public License v1.0
// which accompanies this distribution, and is available at
// http://www.eclipse.org/legal/epl-v10.html
///////////////////////////////////////////////////////////////////////////////
#include "ProjectGenHelper.hh"
#include "../common/memory.h"

#include "error.h"
#include <limits>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const std::string ProjectDescriptor::emptyString = std::string();

ProjectDescriptor::ProjectDescriptor(const char * name) :
  projectName(std::string(name)),
  tpdFileName(),
  targetExecutableName(),
  projectAbsTpdDir(),
  projectAbsWorkingDir(),
  projectWorkingDir(),
  library(false),
  dynamicLinked(false),
  referencedProjects(),
  refProjWorkingDirs(),
  libSearchPaths(),
  linkerLibraries(),
  ttcn3ModuleNames(),
  asn1ModuleNames(),
  userSources(),
  userHeaders(),
  ttcnPP(),
  initialized(false)
{}

void ProjectDescriptor::cleanUp()
{
  referencedProjects.clear();
  refProjWorkingDirs.clear(),
  libSearchPaths.clear();
  linkerLibraries.clear();
  ttcn3ModuleNames.clear();
  asn1ModuleNames.clear();
  userSources.clear();
  userHeaders.clear();
  ttcnPP.clear();
}

bool ProjectDescriptor::isInitialized()
{
  if (!projectName.empty() &&
      !targetExecutableName.empty() &&
      !projectAbsTpdDir.empty() &&
      !projectAbsWorkingDir.empty() &&
      !projectWorkingDir.empty())
    initialized = true;
  return initialized;
}

void ProjectDescriptor::setTPDFileName(const char* name)
{
  const char SEPARATOR = '/';
  std::string fileName(name);
  size_t refProjPos = fileName.find_last_of(SEPARATOR);
  if (std::string::npos == refProjPos) {
    tpdFileName = fileName;
  }
  else {
    tpdFileName = fileName.substr(refProjPos + 1);
  }
}

void ProjectDescriptor::setProjectAbsWorkingDir(const char* name)
{
  if (!name) {
    ERROR("No path was given to the working directory. Check if 'r' flag is set ");
    return;
  }
  projectAbsWorkingDir = std::string(name);
  ProjectGenHelper::Instance().setRootDirOS(name);
}

void ProjectDescriptor::addToReferencedProjects(const char* refProjName)
{
  std::vector<std::string>::iterator it;
  for (it = referencedProjects.begin(); it != referencedProjects.end(); ++it) {
    if (*it  == std::string(refProjName)) return;
  }
  referencedProjects.push_back(std::string(refProjName));
}

void ProjectDescriptor::addToRefProjWorkingDirs(const std::string& subProjDir)
{
  std::vector<std::string>::iterator it;
  for (it = refProjWorkingDirs.begin(); it != refProjWorkingDirs.end(); ++it) {
    if (*it  == subProjDir) return;
  }
  refProjWorkingDirs.push_back(subProjDir);
}

bool ProjectDescriptor::hasLinkerLibTo(const std::string& refProjName) const
{
  ProjectDescriptor* refProj = ProjectGenHelper::Instance().getTargetOfProject(refProjName.c_str());
  for (size_t i = 0; i < referencedProjects.size(); ++i){
    if (refProj && refProj->library) return true;
  }
  return false;
}

bool ProjectDescriptor::hasLinkerLib(const char* libName) const
{
  std::string linkerLibName(libName);
  std::vector<std::string>::const_iterator it;
  for (it = linkerLibraries.begin(); it != linkerLibraries.end(); ++it) {
    if (*it  == linkerLibName) return true;
  }
  return false;
}

void ProjectDescriptor::addToLibSearchPaths(const char* libSearchPath)
{
  std::string searchPath(libSearchPath);
  std::vector<std::string>::iterator it;
  for (it = libSearchPaths.begin(); it != libSearchPaths.end(); ++it) {
    if (*it  == searchPath) return;
  }
  libSearchPaths.push_back(libSearchPath);
}

void ProjectDescriptor::addToLinkerLibs(const char* linkerLibs)
{
  std::string llibs(linkerLibs);
  std::vector<std::string>::iterator it;
  for (it = linkerLibraries.begin(); it != linkerLibraries.end(); ++it) {
    if (*it  == llibs) return;
  }
  linkerLibraries.push_back(linkerLibs);
}

size_t ProjectDescriptor::getLibSearchPathIndex(const std::string& subProjName) const
{

  for (size_t i = 0; i < libSearchPaths.size(); ++i) {
    if (std::string::npos != libSearchPaths[i].find(subProjName))
      return i;
  }
  return std::numeric_limits<unsigned int>::max();
}

const char* ProjectDescriptor::getLibSearchPath(const std::string& subProjName) const
{
  for (size_t i = 0; i < libSearchPaths.size(); ++i) {
    if (std::string::npos != libSearchPaths[i].find(subProjName))
      return libSearchPaths[i].c_str();
  }
  return NULL;
}

bool ProjectDescriptor::hasTtcn3ModuleName(const char* moduleName) const
{
  std::string modName(moduleName);
  std::vector<std::string>::const_iterator it;
  for (it = ttcn3ModuleNames.begin(); it != ttcn3ModuleNames.end(); ++it) {
    if (*it == modName) return true;
  }
  return false;
}

bool ProjectDescriptor::hasAsn1ModuleName(const char* moduleName) const
{
  std::string modName(moduleName);
  std::vector<std::string>::const_iterator it;
  for (it = asn1ModuleNames.begin(); it != asn1ModuleNames.end(); ++it) {
    if (*it == modName) return true;
  }
  return false;
}

bool ProjectDescriptor::hasUserSource(const char* userSourceName) const
{
  std::string sourceName(userSourceName);
  std::vector<std::string>::const_iterator it;
  for (it = userSources.begin(); it != userSources.end(); ++it) {
    if (*it == sourceName) return true;
  }
  return false;
}

bool ProjectDescriptor::hasUserHeader(const char* userHeaderName) const
{
  std::string headerName(userHeaderName);
  std::vector<std::string>::const_iterator it;
  for (it = userHeaders.begin(); it != userHeaders.end(); ++it) {
    if (*it == headerName) return true;
  }
  return false;
}

bool ProjectDescriptor::hasTtcn3PP(const char* ttcnPPName) const
{
  std::string ttcnPPFile(ttcnPPName);
  std::vector<std::string>::const_iterator it;
  for (it = ttcnPP.begin(); it != ttcnPP.end(); ++it) {
    if (*it == ttcnPPFile) return true;
  }
  return false;
}

bool ProjectDescriptor::hasXSDModuleName(const char* xsdName) const
{
  std::string modName(xsdName);
  std::vector<std::string>::const_iterator it;
  for (it = xsdModuleNames.begin(); it != xsdModuleNames.end(); ++it) {
    if (*it == modName) return true;
  }
  return false;
}

std::string ProjectDescriptor::setRelativePathTo(const std::string& absPathTo)
{
  if (projectAbsWorkingDir.empty()) return std::string();
  const char SEPARATOR = '/';
  if (projectAbsWorkingDir.at(0) != SEPARATOR || absPathTo.at(0) != SEPARATOR) 
    ERROR("Expecting absolute path to generate LinkerLibSearchPath ");
  size_t length = projectAbsWorkingDir.size() > absPathTo.size() ? absPathTo.size() : projectAbsWorkingDir.size();
  size_t lastSlash = 0;
  size_t i;
  for(i = 0; i < length && projectAbsWorkingDir.at(i) == absPathTo.at(i); ++i) {
    if (projectAbsWorkingDir.at(i) == SEPARATOR && absPathTo.at(i) == SEPARATOR) {
      lastSlash = i; // the same path until now...
    }
  }
  if (length == i) {  // got subdirectory
    if (projectAbsWorkingDir == absPathTo) {
      return std::string("."); // the same paths were given 
    }
    else if ((projectAbsWorkingDir.size() > absPathTo.size() && projectAbsWorkingDir.at(length) == SEPARATOR) ||
       (projectAbsWorkingDir.size() < absPathTo.size() && absPathTo.at(length) == SEPARATOR))
      lastSlash = length;
  }

  size_t slashCount = 0;
  for (i = lastSlash; i < projectAbsWorkingDir.size(); ++i) {
    if (projectAbsWorkingDir.at(i) == SEPARATOR)
      ++slashCount;
  }

  std::string relPath;
  const std::string upDir("../");
  for (i = 0; i < slashCount; ++i)
    relPath.append(upDir);

  std::string pathTo = absPathTo.substr(lastSlash+1); // we left the heading slash
  relPath.append(pathTo);
  return std::string(relPath);
}

void ProjectDescriptor::print()
{
  fprintf( stderr, "project name %s and it is %s initialized\n", projectName.c_str(), isInitialized() ? "" : "not");
  fprintf( stderr, "  target executable name %s\n",targetExecutableName.c_str());
  fprintf( stderr, "  project abs TPD dir %s\n", projectAbsTpdDir.c_str());
  fprintf( stderr, "  project abs working dir %s\n", projectAbsWorkingDir.c_str());
  fprintf( stderr, "  project working dir %s\n", projectWorkingDir.c_str());
  fprintf( stderr, "  project is %s\n", library ? "Library" : "Executable");
  fprintf( stderr, "  project linking is %s\n", dynamicLinked ? "dynamic" : "static");
  std::vector<std::string>::iterator it;
  for (it = referencedProjects.begin(); it != referencedProjects.end(); ++it) {
    fprintf( stderr, "    Referenced project %s\n",(*it).c_str());
  }
  for (it = refProjWorkingDirs.begin(); it != refProjWorkingDirs.end(); ++it) {
    fprintf( stderr, "    Working dir of referenced project %s\n",(*it).c_str());
  }
  for (it = linkerLibraries.begin(); it != linkerLibraries.end(); ++it) {
    fprintf( stderr, "    Linker library %s\n", (*it).c_str());
  }
  for (it = libSearchPaths.begin(); it != libSearchPaths.end(); ++it) {
    fprintf( stderr, "    Linker lib search path %s\n", (*it).c_str());
  }
  for (it = ttcn3ModuleNames.begin(); it != ttcn3ModuleNames.end(); ++it) {
    fprintf( stderr, "    TTCN3 Module Name: %s\n", (*it).c_str());
  }
  for (it = asn1ModuleNames.begin(); it != asn1ModuleNames.end(); ++it) {
    fprintf( stderr, "    ASN1 Module Name: %s\n", (*it).c_str());
  }
  for (it = userSources.begin(); it != userSources.end(); ++it) {
    fprintf( stderr, "    Source Name: %s\n", (*it).c_str());
  }
  for (it = userHeaders.begin(); it != userHeaders.end(); ++it) {
    fprintf( stderr, "    Header Name: %s\n", (*it).c_str());
  }
  for (it = ttcnPP.begin(); it != ttcnPP.end(); ++it) {
    fprintf( stderr, "    TTCN PP Name: %s\n", (*it).c_str());
  }
  fprintf( stderr, "\n");
}

ProjectGenHelper& ProjectGenHelper::Instance()
{
  static ProjectGenHelper singleton;
  return singleton;
}

const std::string ProjectGenHelper::emptyString = std::string();

ProjectGenHelper::ProjectGenHelper() :
  nameOfTopLevelProject(),
  rootDirOS(),
  relPathToRootDirOS(),
  Zflag(false),
  Wflag(false),
  Hflag(false),
  projs(),
  checkedProjs(),
  projsNameWithAbsPath()
{}

void ProjectGenHelper::addTarget(const char* projName)
{
  if (!Zflag) return;
  if (projs.end() != projs.find(std::string(projName))) return; // we have it 
  ProjectDescriptor newLib(projName);
  projs.insert(std::pair<std::string, ProjectDescriptor> (std::string(projName), newLib));
}

void ProjectGenHelper::setToplevelProjectName(const char* name)
{
  if (!nameOfTopLevelProject.empty()) return;
   nameOfTopLevelProject = std::string(name);
}

void ProjectGenHelper::setRootDirOS( const char* name)
{
  if (rootDirOS.empty()) {
    rootDirOS = std::string(name);
  }
  else { //compare the 2 string and get the common part
    const char* root = rootDirOS.c_str();
    const char* head = root;
    for (; *root++ == *name++; ) ;
    size_t length = root - head - 1; //minus the non-matching
    if (rootDirOS.size() > length) {
      rootDirOS.resize(length);
    }
  }
}

const std::string& ProjectGenHelper::getRootDirOS(const char* name)
{
  ProjectDescriptor* proj = getProject(name);
  if (!proj) return emptyString;
  relPathToRootDirOS = proj->setRelativePathTo(rootDirOS);
  return relPathToRootDirOS;
}

ProjectDescriptor* ProjectGenHelper::getTargetOfProject(const char* projName)
{
  if (!Zflag) return NULL;
  if (projs.end() == projs.find(std::string(projName))) return NULL;
  return getProject(projName);
}

const ProjectDescriptor* ProjectGenHelper::getTargetOfProject(const char* projName) const
{
  if (!Zflag) return NULL;
  if (projs.end() == projs.find(std::string(projName))) return NULL;
  return getProject(projName);
}

ProjectDescriptor* ProjectGenHelper::getProjectDescriptor(const char* targetName)
{
  if (!Zflag) return NULL;
  for (std::map<std::string, ProjectDescriptor>::iterator it = projs.begin(); it != projs.end(); ++it) {
    if ((it->second).getTargetExecName() == std::string(targetName))
      return &(it->second);
  }
  return NULL;
}

std::map<std::string, ProjectDescriptor>::const_iterator ProjectGenHelper::getHead() const
{
  return projs.begin();
}

std::map<std::string, ProjectDescriptor>::const_iterator ProjectGenHelper::getEnd() const
{
  return projs.end();
}

void ProjectGenHelper::addTtcn3ModuleToProject(const char* projName, const char* moduleName)
{
  if (!Zflag) return;
  if (projs.end() == projs.find(std::string(projName))) return;
  ProjectDescriptor* proj = getProject(projName);
  if (proj && !proj->hasTtcn3ModuleName(moduleName)) {
     proj->addTtcn3ModuleName(moduleName);
  }
}

void ProjectGenHelper::addAsn1ModuleToProject(const char* projName, const char* moduleName)
{
  if (!Zflag) return;
  if (projs.end() == projs.find(std::string(projName))) return;
  ProjectDescriptor* proj = getProject(projName);
  if (proj &&!proj->hasAsn1ModuleName(moduleName)) {
     proj->addAsn1ModuleName(moduleName);
  }
}

void ProjectGenHelper::addUserSourceToProject(const char* projName, const char* userSourceName)
{
  if (!Zflag) return;
  if (projs.end() == projs.find(std::string(projName))) return;
  ProjectDescriptor* proj = getProject(projName);
  if (proj && !proj->hasUserSource(userSourceName)) {
     proj->addUserSource(userSourceName);
  }
}

void ProjectGenHelper::addUserHeaderToProject(const char* projName, const char* userHeaderName)
{
  if (!Zflag) return;
  if (projs.end() == projs.find(std::string(projName))) return;
  ProjectDescriptor* proj = getProject(projName);
  if (proj && !proj->hasUserHeader(userHeaderName)) {
    proj->addUserHeader(userHeaderName);
  }
}

void ProjectGenHelper::addTtcnPPToProject(const char* projName, const char* ttcnPPName)
{
  if (!Zflag) return;
  if (projs.end() == projs.find(std::string(projName))) return;
  ProjectDescriptor* proj = getProject(projName);
  if (proj && !proj->hasTtcn3PP(ttcnPPName)) {
    proj->addTtcn3PP(ttcnPPName);
  }
}

void ProjectGenHelper::addXSDModuleToProject(const char* projName, const char* xsdModuleName)
{
  if (!Zflag) return;
  if (projs.end() == projs.find(std::string(projName))) return;
  ProjectDescriptor* proj = getProject(projName);
  if (proj && !proj->hasXSDModuleName(xsdModuleName)) {
    proj->addXSDModuleName(xsdModuleName);
  }
}

void ProjectGenHelper::generateRefProjectWorkingDirsTo(const char* projName)
{
  if (!Zflag) return;
  std::map<std::string,ProjectDescriptor>::iterator iter = projs.find(projName);
  if (projs.end() == iter) {
    ERROR("Project \"%s\" is not found in the project hierarchy ", projName);
    return;
  }
  if (nameOfTopLevelProject != (iter->second).getProjectName()) {
    ERROR("Project \"%s\" is not the on the top-level ", projName);
    return;
  }
  ProjectDescriptor* proj = &(iter->second); // the Top level project

  for (size_t i = 0; i < proj->numOfReferencedProjects(); ++i) {
    const std::string& refProjName = proj->getReferencedProject(i);
    ProjectDescriptor* refProj = getTargetOfProject(refProjName.c_str());
    if (!refProj) return; // for sure...
    const std::string& absWorkingDir = refProj->getProjectAbsWorkingDir();
    if (!absWorkingDir.empty()) {
      std::string relPath =  proj->setRelativePathTo(absWorkingDir);
      proj->addToRefProjWorkingDirs(relPath);
    }
  }
}

size_t ProjectGenHelper::numOfLibs() const
{
  if (!Zflag) return 0;
  size_t num = 0;
  for (std::map<std::string, ProjectDescriptor>::const_iterator it = projs.begin(); it != projs.end(); ++it) {
    if ((it->second).isLibrary()) {
      ++num;
    }
  }
  return num;
}

struct CompareStr
{
  bool operator () (const char* lhs, const char* rhs) {
    int ret = strcmp(lhs, rhs);
    return (0 > ret);
  }
};

void ProjectGenHelper::getExternalLibs(std::vector<const char*>& extLibs)
{
  if (!Zflag) return;
  std::map<const char*, const char*, CompareStr> libs;
  for (std::map<std::string, ProjectDescriptor>::iterator it = projs.begin(); it != projs.end(); ++it) {
    if ((it->second).numOfLinkerLibs() > 0) {
      for (size_t i = 0; i < (it->second).numOfLinkerLibs(); ++i) {
        const char* key = (it->second).getLinkerLib(i);
        const char* value = (it->second).getProjectName().c_str();
        libs.insert(std::pair<const char*,const char*>(key,value)); // filter duplicates
      }
    }
  }
  std::map<const char*, const char*>::iterator it;
  for (it = libs.begin(); it != libs.end(); ++it) {
    extLibs.push_back(it->first);
  }
}

void ProjectGenHelper::getExternalLibSearchPaths(std::vector<const char*>& extLibPaths)
{
  if (!Zflag) return;
  std::map<const char*, const char*, CompareStr> libPaths;
  for (std::map<std::string, ProjectDescriptor>::iterator it = projs.begin(); it != projs.end(); ++it) {
    if ((it->second).numOfLibSearchPaths() > 0) {
      for (size_t i = 0; i < (it->second).numOfLibSearchPaths(); ++i) {
        const char* key = (it->second).getLibSearchPath(i);
        const char* value = (it->second).getProjectName().c_str();
        libPaths.insert(std::pair<const char*,const char*>(key,value)); // filter duplicates
      }
    }
  }
  std::map<const char*, const char*>::iterator it;
  for (it = libPaths.begin(); it != libPaths.end(); ++it) {
    extLibPaths.push_back(it->first);
  }
}

bool ProjectGenHelper::hasReferencedProject()
{
  if (!Zflag) return false;
  ProjectDescriptor* topLevel = getTargetOfProject(nameOfTopLevelProject.c_str());
  if (topLevel && topLevel->numOfReferencedProjects()) return true;
  return false;
}

bool ProjectGenHelper::isTtcn3ModuleInLibrary(const char* moduleName) const
{
  if (!Zflag) return false;
  for (std::map<std::string, ProjectDescriptor>::const_iterator it = projs.begin(); it != projs.end(); ++it) {
    if ((it->second).hasTtcn3ModuleName(moduleName) && (it->second).isLibrary()) return true;
  }
  return false;
}

bool ProjectGenHelper::isAsn1ModuleInLibrary(const char* moduleName) const
{
  if (!Zflag) return false;
  for (std::map<std::string, ProjectDescriptor>::const_iterator it = projs.begin(); it != projs.end(); ++it) {
    if ((it->second).hasAsn1ModuleName(moduleName) && (it->second).isLibrary()) return true;
  }
  return false;
}

bool ProjectGenHelper::isSourceFileInLibrary(const char* fileName) const
{
  if (!Zflag || NULL == fileName) return false;
  for (std::map<std::string, ProjectDescriptor>::const_iterator it = projs.begin(); it != projs.end(); ++it) {
    if ((it->second).hasUserSource(fileName) && (it->second).isLibrary()) return true;
  }
  return false;
}

bool ProjectGenHelper::isHeaderFileInLibrary(const char* fileName) const
{
  if (!Zflag || NULL == fileName) return false;

  for (std::map<std::string, ProjectDescriptor>::const_iterator it = projs.begin(); it != projs.end(); ++it) {
    if ((it->second).hasUserHeader(fileName) && (it->second).isLibrary()) return true;
  }
  return false;
}

bool ProjectGenHelper::isTtcnPPFileInLibrary(const char* fileName) const
{
  if (!Zflag || NULL == fileName) return false;

  for (std::map<std::string, ProjectDescriptor>::const_iterator it = projs.begin(); it != projs.end(); ++it) {
    if ((it->second).hasTtcn3PP(fileName) && (it->second).isLibrary()) return true;
  }
  return false;
}

bool ProjectGenHelper::isXSDModuleInLibrary(const char* fileName) const
{
  if (!Zflag || NULL == fileName) return false;

  for (std::map<std::string, ProjectDescriptor>::const_iterator it = projs.begin(); it != projs.end(); ++it) {
    if ((it->second).hasXSDModuleName(fileName) && (it->second).isLibrary()) return true;
  }
  return false;
}

bool ProjectGenHelper::isCPPSourceFile(const char* fileName) const
{
  std::string fnStr(fileName);
  size_t pos = fnStr.find_last_of('.');
  if (std::string::npos == pos) return false;
  const std::string EXT_CC("cc");
  const std::string EXT_CPP("cpp");
  int length = 0;
  if (std::string::npos != fnStr.find (EXT_CC, pos + 1))
    length = EXT_CC.size();
  else if (std::string::npos != fnStr.find (EXT_CPP, pos + 1))
    length = EXT_CPP.size();

  if (length && fnStr.size() == pos + length + 1)
    return true;
  else 
    return false;
}

bool ProjectGenHelper::isCPPHeaderFile(const char* fileName) const
{
  std::string fnStr(fileName);
  size_t pos = fnStr.find_last_of('.');
  if (std::string::npos == pos) return false;
  const std::string EXT_HPP("hpp");
  const std::string EXT_HH("hh");
  const std::string EXT_H("h");
  int length = 0;
  if (std::string::npos != fnStr.find (EXT_HH, pos + 1))
    length = EXT_HH.size();
  else if (std::string::npos != fnStr.find (EXT_HPP, pos + 1))
    length = EXT_HPP.size();
  else if (std::string::npos != fnStr.find (EXT_H, pos + 1))
    length = EXT_H.size();

  if (length && fnStr.size() == pos + length + 1)
    return true;
  else
    return false;
}

bool ProjectGenHelper::isTtcnPPFile(const char* fileName) const
{
  std::string fnStr(fileName);
  size_t pos = fnStr.find_last_of('.');
  if (std::string::npos == pos) return false;
  const std::string EXT_TTCNPP("ttcnpp");
  int length = 0;
  if (std::string::npos != fnStr.find (EXT_TTCNPP, pos + 1))
    length = EXT_TTCNPP.size();

  if (length && fnStr.size() == pos + length + 1)
    return true;
  else
    return false;
}

void ProjectGenHelper::print()
{
  if (!Zflag) return;
  fprintf(stderr, "Top Level project : %s\n", nameOfTopLevelProject.c_str());
  for (std::map<std::string, ProjectDescriptor>::iterator it = projs.begin(); it != projs.end(); ++it) {
    (it->second).print();
  }
}

bool ProjectGenHelper::sanityCheck()
{
  if (!Zflag) return true;
  bool ret = true;
// if toplevel is a dynamic linked executable (not library) all executable shall set to the same linking method
  {
    ProjectDescriptor* topLevel = getTargetOfProject(nameOfTopLevelProject.c_str());
    bool isDynamicLinked = topLevel->getLinkingStrategy();
    if (!topLevel->isLibrary() && isDynamicLinked) { // dynamic linked executable
      for (std::map<std::string, ProjectDescriptor>::iterator it = projs.begin(); it != projs.end(); ++it) {
        if (!(it->second).isLibrary()) { //if exectubale 
          if (isDynamicLinked != (it->second).getLinkingStrategy()) {
             ERROR("project \"%s\" is set to %s linking. Sub project \"%s\" is set to %s linking. "
                   "All sub-level executable shall be set to the %s's type.",
                   nameOfTopLevelProject.c_str(),
                   isDynamicLinked ? "dynamic" : "static",
                   ((it->second).getProjectName()).c_str(),
                   isDynamicLinked ? "static" : "dynamic",
                   nameOfTopLevelProject.c_str());
             ret = false;
          }
        }
      }
    }
  }

// under a dynamic linked library every project shall be linked dynamic library too.
  {
    checkedProjs.clear();
    bool found = false; // search for executable under dynamic linked library
    char* execName = NULL;
    for (std::map<std::string, ProjectDescriptor>::reverse_iterator rit = projs.rbegin(); rit != projs.rend(); ++rit) {
      if ((rit->second).isLibrary() && (rit->second).getLinkingStrategy()) { //dynamic library 
        ProjectDescriptor& proj = rit->second;
        std::vector<std::string> history;
        history.push_back(proj.getProjectName());
        found = DynamicLibraryChecker(&proj, found, &execName, history);
        if (found) {
           ERROR("Project \"%s\" is dynamic linked library. Sub project \"%s\" is executable.\n"
                 "in TPD file, %s's all sub-level defaultTarget shall be set library too.",
                 proj.getProjectName().c_str(), execName, proj.getProjectName().c_str());
           ret = false;
           break;
        }
      }
    }
  }

  return ret;
}

ProjectDescriptor* ProjectGenHelper::getProject(const char* projName)
{
  if (!projName) return NULL;
  for (std::map<std::string, ProjectDescriptor>::iterator it = projs.begin(); it != projs.end(); ++it) {
    if (it->first == std::string(projName)) {
      return &(it->second);
    }
  }
  return NULL;
}

const ProjectDescriptor* ProjectGenHelper::getProject(const char* projName) const
{
  if (!projName) return NULL;
  for (std::map<std::string, ProjectDescriptor>::const_iterator it = projs.begin(); it != projs.end(); ++it) {
    if (it->first == std::string(projName)) {
      return &(it->second);
    }
  }
  return NULL;
}


void ProjectGenHelper::cleanUp()
{
  if (!Zflag) return;
  checkedProjs.clear();
  for (std::map<std::string, ProjectDescriptor>::iterator it = projs.begin(); it != projs.end(); ++it) {
    (it->second).cleanUp();
  }
}

bool ProjectGenHelper::DynamicLibraryChecker(const ProjectDescriptor* desc,
                                       bool& found,
                                       char** executableName,
                                       std::vector<std::string>& history)
{
  if (found || !desc) return true;
  for (size_t i = 0; i < desc->numOfReferencedProjects(); ++i) {
    char* refProjName = const_cast<char*> (desc->getReferencedProject(i).c_str());
    const ProjectDescriptor* subProj = getTargetOfProject(refProjName);
    if (0 == checkedProjs.count(subProj->getProjectName())) {
      if (subProj->isLibrary()) {
        // Check if we already checked this subproject.
        bool inHistory = false;
        for (size_t j = 0; j < history.size(); j++) {
          if (history[j] == subProj->getProjectName()) {
            inHistory = true;
            ERROR("Project hierarchy has circular references which is not supported when the improved linking method is used.\n"
                  "For further information see the TITAN referenceguide.");
          }
        }
        if (!inHistory) {
          found = DynamicLibraryChecker(subProj, found, executableName, history);
          history.push_back(subProj->getProjectName());
        }
      }
      else { // search for executable under dynamic linked library
        found = true;
        *executableName = refProjName;
        break;
      }
    }
  }
  // it is checked, no such executable was found. Store it not to iterate again
  if (!found)
    checkedProjs.insert(std::pair<const std::string, const ProjectDescriptor*> 
                       (desc->getProjectName(), desc));
  return found;
}

bool ProjectGenHelper::insertAndCheckProjectName(const char *absPath, const char *projName) {
  if (projName == NULL) return true;
  std::string absPathStr = std::string(absPath);
  std::map<const std::string, const std::string>::iterator it = projsNameWithAbsPath.find(absPathStr);
   if (it == projsNameWithAbsPath.end()) {
    projsNameWithAbsPath.insert(std::pair<const std::string, const std::string>(absPathStr, std::string(projName)));
    return true;
  } else {
    if (it->second != projName) {
      return false;
    }
  }
  return true;
}
