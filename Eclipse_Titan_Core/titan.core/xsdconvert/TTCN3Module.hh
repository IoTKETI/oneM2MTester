/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Godar, Marton
 *   Raduly, Csaba
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
#ifndef TTCN3MODULE_HH_
#define TTCN3MODULE_HH_

#include "XMLParser.hh"
#include "GeneralTypes.hh"
#include "GeneralFunctions.hh"
#include "TTCN3ModuleInventory.hh"

class TTCN3ModuleInventory;
class RootType;

//Used only in type substitution, when we have to change the type or a builtintype
//due to type substitution
struct typeNameDepList {
  Mstring type;
  List<SimpleType*> nameDepList;
  //Used only with builtInTypes
  ComplexType* typeSubsGroup;
};

/**
 * Type that contains information about one TTCN-3 module
 * and performs the generation of that module
 *
 * Types defined in the module are stored in a list
 *
 */
class TTCN3Module {
  /**
   * this module is connected with this parser object
   */
  XMLParser * parser; // no responsibility for this member

  /**
   * Name of the XML schema that this module is originating from
   */
  Mstring schemaname;
  /**
   * Name of the TTCN-3 module (derived from targetNamespace)
   */
  Mstring modulename;
  /**
   * contains all main types defined in the module
   */
  List<RootType*>definedTypes;
  /**
   * contains all constant definitions defined in the module
   */
  List<RootType*>constantDefs;
  /**
   * Type of the currently processed main type
   */
  ConstructType actualXsdConstruct;

  /**
   * Attribute values of XML declaration
   */
  Mstring xsdVersion;
  Mstring xsdEncoding;
  int xsdStandalone;
  /**
   * Attribute values of <schema> tag
   */
  Mstring targetNamespace;
  Mstring targetNamespace_connectedPrefix;
  List<NamespaceType> declaredNamespaces;
  FormValue elementFormDefault;
  FormValue attributeFormDefault;
  BlockValue blockDefault;

  List<const TTCN3Module*> importedModules; // pointers not owned

  //Used only in type substitution, stores every possibly substituted type definitions
  List<ComplexType*> storedTypeSubstitutions; //pointers not owned

  //Used only in type substitution, stores every element that references a type
  List<typeNameDepList> element_types;

  List<Mstring> variant;
  
  List<Mstring> xmlSchemaPrefixes;

  bool moduleNotIntoFile;
  bool moduleNotIntoNameConversion;
  
  unsigned int const_counter;

  TTCN3Module & operator=(const TTCN3Module &); // not implemented
  TTCN3Module(const TTCN3Module &); // not implemented
public:
  TTCN3Module(const char * a_filename, XMLParser * a_parser);
  ~TTCN3Module();

  void goodbyeParser() {
    parser = 0;
  }

  void loadValuesFromXMLDeclaration(const char *a_version,
      const char *a_encoding, int a_standalone);
  void loadValuesFromSchemaTag(const Mstring& a_targetNamespace, List<NamespaceType> declaredNamespaces,
      FormValue a_elementFormDefault,
      FormValue a_attributeFormDefault,
      BlockValue a_blockDefault);

  void generate_TTCN3_fileinfo(FILE * file);
  void generate_TTCN3_modulestart(FILE * file);
  void generate_TTCN3_included_types(FILE * file);
  void generate_TTCN3_import_statements(FILE * file);
  void generate_TTCN3_types(FILE * file);
  void generate_with_statement(FILE * file, List<NamespaceType> used_namespaces);

  const Mstring & getSchemaname() const {
    return schemaname;
  }

  const Mstring & getTargetNamespace() const {
    return targetNamespace;
  }

  const Mstring & getTargetNamespaceConnector() const {
    return targetNamespace_connectedPrefix;
  }

  const Mstring & getModulename() const {
    return modulename;
  }

  void setSchemaname(const Mstring& name) {
    schemaname = name;
  }

  void setTargetNamespace(const Mstring& tns) {
    targetNamespace = tns;
  }

  FormValue getElementFormDefault() const {
    return elementFormDefault;
  }

  void setElementFormDefault(FormValue value) {
    elementFormDefault = value;
  }

  FormValue getAttributeFormDefault() const {
    return attributeFormDefault;
  }

  void setAttributeFormDefault(FormValue value) {
    attributeFormDefault = value;
  }

  BlockValue getBlockDefault() const {
      return blockDefault;
  }

  ConstructType getActualXsdConstruct() const {
    return actualXsdConstruct;
  }

  void setActualXsdConstruct(ConstructType c) {
    actualXsdConstruct = c;
  }

  void addAnnotation();
  void addMainType(const ConstructType typeOfMainType);
  void addMainType(RootType * type) { definedTypes.push_back(type); }
  void addConstant(RootType * constant) { constantDefs.push_back(constant); }
  
  bool hasDefinedMainType() const {
    return !definedTypes.empty();
  }
  void replaceLastMainType(RootType * t);

  const List<RootType*> & getDefinedTypes() const {
    return definedTypes;
  }
  
   List<RootType*> & getConstantDefs() {
    return constantDefs;
  }

  RootType & getLastMainType() {
    return *definedTypes.back();
  }

  bool isnotIntoNameConversion() const {
    return moduleNotIntoNameConversion;
  }

  void notIntoNameConversion() {
    moduleNotIntoNameConversion = true;
  }

  bool isnotIntoFile() const {
    return moduleNotIntoFile;
  }

  void notIntoFile() {
    moduleNotIntoFile = true;
  }

  const List<NamespaceType> & getDeclaredNamespaces() const {
    return declaredNamespaces;
  }

  void addImportedModule(const TTCN3Module *mod) {
    importedModules.push_back(mod);
  }

  const List<const TTCN3Module*> & getImportedModules() const {
    return importedModules;
  }

  List<ComplexType*> & getStoredTypeSubstitutions() {
    return storedTypeSubstitutions;
  }

  void addStoredTypeSubstitution(ComplexType * type){
    storedTypeSubstitutions.push_back(type);
  }

  List<typeNameDepList> & getElementTypes() {
    return element_types;
  }
  
  const List<Mstring> & getxmlSchemaPrefixes() const {
    return xmlSchemaPrefixes;
  }

  void addElementType(const Mstring& type, SimpleType* st) {
    List<typeNameDepList>::iterator it = element_types.begin();
    for(; it; it = it->Next){
        if(it->Data.type == type && st != NULL){
            it->Data.nameDepList.push_back(st);
            break;
        }
    }
    //New type that has not been in the element_types before
    if(it == NULL){
      typeNameDepList list;
      list.type = type;
      list.typeSubsGroup = NULL;
      if(st != NULL){
        list.nameDepList.push_back(st);
      }
      element_types.push_back(list);
    }
  }

  /// Compute the TTCN-3 module name
  void TargetNamespace2ModuleName();

  friend bool compareModules(TTCN3Module * lhs, TTCN3Module * rhs);
  
  unsigned int getConstCounter() {
    return const_counter;
  }
  
  void increaseConstCounter() {
    ++const_counter;
  }

  void dump() const;
};

inline bool compareModules(TTCN3Module * lhs, TTCN3Module * rhs) {
  return lhs->targetNamespace < rhs->targetNamespace;
}

#endif /* TTCN3MODULECONTAINER_HH_ */
