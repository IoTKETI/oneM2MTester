///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2000-2017 Ericsson Telecom AB
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the Eclipse Public License v1.0
// which accompanies this distribution, and is available at
// http://www.eclipse.org/legal/epl-v10.html
///////////////////////////////////////////////////////////////////////////////
#ifndef _LIB_GEN_HELPER_HH
#define _LIB_GEN_HELPER_HH
#include <string>
#include <map>
#include <vector>
#include <cstdio>
class ProjectGenHelper;
class ProjectDescriptor {
public:
  explicit ProjectDescriptor(const char* name);
  ~ProjectDescriptor() { cleanUp(); };

  const std::string& getProjectName() const { return projectName; }
  void setTPDFileName( const char* name);
  const std::string& getTPDFileName() const { return tpdFileName; }
  void setTargetExecName(const char* name) { targetExecutableName = std::string(name); }
  const std::string& getTargetExecName() const { return targetExecutableName; }
  void setProjectAbsTpdDir(const char* name) { projectAbsTpdDir = std::string(name); }
  const std::string& getProjectAbsTpdDir() const { return projectAbsTpdDir; }
  void setProjectAbsWorkingDir(const char* name);
  void setProjectWorkingDir(const char* name) { projectWorkingDir = std::string(name); }
  const std::string& getProjectAbsWorkingDir() const { return projectAbsWorkingDir; }
  const std::string& getProjectWorkingDir() const { return projectWorkingDir; }
  void setLinkingStrategy(bool strategy) { dynamicLinked = strategy; }
  bool getLinkingStrategy() const { return dynamicLinked; }
  void setLibrary(bool isLib) { library = isLib; }
  bool isLibrary() const { return library; }
  bool isInitialized();
  void addToReferencedProjects(const char* refProjName);
  void addToRefProjWorkingDirs(const std::string& subProjDir);
  bool hasLinkerLibTo(const std::string& refProjName) const; 
  bool hasLinkerLib(const char* libName) const; // Linker Lib got from TPD
  void addToLibSearchPaths(const char* libSearchPath);
  void addToLinkerLibs(const char* linkerLibs);
  void print();
  void cleanUp();
  size_t numOfReferencedProjects() const { return referencedProjects.size(); };
  size_t numOfRefProjWorkingDirs() const { return refProjWorkingDirs.size(); };
  size_t numOfLibSearchPaths() const { return libSearchPaths.size(); };
  size_t numOfLinkerLibs() const { return linkerLibraries.size(); };
  const std::string& getReferencedProject(size_t index) const 
    { return index < referencedProjects.size() ? referencedProjects[index] : emptyString; };
  const std::string& getRefProjWorkingDir(size_t index) const 
    { return index < refProjWorkingDirs.size() ? refProjWorkingDirs[index] : emptyString; };
  const char* getLibSearchPath(const std::string& subProjName) const;
  const char* getLibSearchPath(size_t index) const { return libSearchPaths[index].c_str(); };
  const char* getLinkerLib(const std::string& subProjName) const;
  const char* getLinkerLib(size_t index) const { return linkerLibraries[index].c_str(); };
  size_t getLibSearchPathIndex(const std::string& subProjName) const;
  void setLibSearchPath(size_t index, const std::string& relPath) { libSearchPaths[index] = relPath; };
  void addTtcn3ModuleName(const char* name) { ttcn3ModuleNames.push_back(name); };
  bool hasTtcn3ModuleName(const char* moduleName) const;
  void addAsn1ModuleName(const char* name) { asn1ModuleNames.push_back(name); };
  bool hasAsn1ModuleName(const char* moduleName) const;
  void addUserSource(const char* name) { userSources.push_back(name); };
  bool hasUserSource(const char* userSourceName) const;
  void addUserHeader(const char* name) { userHeaders.push_back(name); };
  bool hasUserHeader(const char* userHeaderName) const;
  void addTtcn3PP(const char* name) { ttcnPP.push_back(name); };
  bool hasTtcn3PP(const char* ttcnPPName) const;
  void addXSDModuleName(const char* name) { xsdModuleNames.push_back(name); };
  bool hasXSDModuleName(const char* xsdName) const;
  std::string setRelativePathTo(const std::string& absPathTo);

private:
  static const std::string emptyString;
  std::string projectName;
  std::string tpdFileName;
  std::string targetExecutableName; //Library or Executable(only the top level)
  std::string projectAbsTpdDir;
  std::string projectAbsWorkingDir;
  std::string projectWorkingDir;
  bool library;
  bool dynamicLinked;
  std::vector<std::string> referencedProjects;
  std::vector<std::string> refProjWorkingDirs;
  std::vector<std::string> libSearchPaths;
  std::vector<std::string> linkerLibraries;
  std::vector<std::string> ttcn3ModuleNames;
  std::vector<std::string> asn1ModuleNames;
  std::vector<std::string> userSources; // *.cc ; *.cpp
  std::vector<std::string> userHeaders; // *.hh ; *.h ; *.hpp
  std::vector<std::string> ttcnPP; // *.ttcnpp
  std::vector<std::string> xsdModuleNames; // .xsd
  bool initialized;
};

class ProjectGenHelper {
public:
  static ProjectGenHelper &Instance();
  ~ProjectGenHelper() { cleanUp(); };
  void setZflag(bool flag) { Zflag = flag; };
  bool getZflag() const { return Zflag; };
  void setWflag(bool flag) { Wflag = flag; };
  bool getWflag() const { return Wflag; };
  void setHflag(bool flag) { Hflag = flag; };
  bool getHflag() const { return Hflag; };
  void setToplevelProjectName(const char* name);
  const std::string& getToplevelProjectName() const { return nameOfTopLevelProject; };
  void setRootDirOS(const char* name);
  const std::string& getRootDirOS(const char* name);
  void addTarget(const char* projName);
  void generateRefProjectWorkingDirsTo(const char* projName);
  void addTtcn3ModuleToProject(const char* projName, const char* moduleName);
  void addAsn1ModuleToProject(const char* projName, const char* moduleName);
  void addUserSourceToProject(const char* projName, const char* userSourceName);
  void addUserHeaderToProject(const char* projName, const char* userHeaderName);
  void addTtcnPPToProject(const char* projName, const char* ttcnPPName);
  void addXSDModuleToProject(const char* projName, const char* xsdModuleName);
  bool isTtcn3ModuleInLibrary(const char* moduleName) const;
  bool isAsn1ModuleInLibrary(const char* moduleName) const;
  bool isSourceFileInLibrary(const char* fileName) const;
  bool isHeaderFileInLibrary(const char* fileName) const;
  bool isTtcnPPFileInLibrary(const char* fileName) const;
  bool isXSDModuleInLibrary(const char* fileName) const;
  ProjectDescriptor* getTargetOfProject(const char* projName);
  const ProjectDescriptor* getTargetOfProject(const char* projName) const;
  ProjectDescriptor* getProjectDescriptor(const char* targetName); //target_executable_name
  std::map<std::string, ProjectDescriptor>::const_iterator getHead() const;
  std::map<std::string, ProjectDescriptor>::const_iterator getEnd() const;
  size_t numOfLibs() const;
  void getExternalLibs(std::vector<const char*>& extLibs);
  void getExternalLibSearchPaths(std::vector<const char*>& extLibPaths);
  bool hasReferencedProject();
  size_t numOfProjects() const { return projs.size();};
  bool isCPPSourceFile(const char* fileName) const;
  bool isCPPHeaderFile(const char* fileName) const;
  bool isTtcnPPFile(const char* fileName) const;
  void print();
  bool sanityCheck(); // tests if the structure generated from TPDs is consistent
  void cleanUp();
  bool insertAndCheckProjectName(const char *absPath, const char *projName);

private:
    ProjectGenHelper();
    ProjectGenHelper(const ProjectGenHelper &rhs);
    ProjectGenHelper &operator=(const ProjectGenHelper &rhs);
    ProjectDescriptor* getProject(const char* projName);
    const ProjectDescriptor* getProject(const char* projName) const;
    bool DynamicLibraryChecker(const ProjectDescriptor* desc,
                         bool& found,
                         char** executableName,
                         std::vector<std::string>& history);
private:
  static ProjectGenHelper& intance;
  static const std::string emptyString;
  std::string nameOfTopLevelProject;
  std::string rootDirOS; // make archive needs the top dir on OS level
  std::string relPathToRootDirOS;
  bool Zflag; // the makefilegen switch wether to use this option at all
  bool Wflag; // prefix woring directory
  bool Hflag; // hierarchical make structure
  std::map<const std::string, ProjectDescriptor> projs;
  std::map<const std::string, const ProjectDescriptor*> checkedProjs;
  // Key is the absolute path name, and the value is the name of the referenced project
  std::map<const std::string, const std::string> projsNameWithAbsPath;
};

#endif // _LIB_GEN_HELPER_HH
