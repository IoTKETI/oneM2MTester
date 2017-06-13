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
#ifndef TTCN3MODULEINVENTORY_HH_
#define TTCN3MODULEINVENTORY_HH_

#include "Mstring.hh"
#include "List.hh"

class TTCN3Module;
class RootType;
class SimpleType;
class ComplexType;

/**
 * Type that contains generated TTCN-3 modules in a list
 * and performs some modifications:
 *
 * 	- conversion of the names of modules
 * 	- finding module name information for import statements
 * 	- reference resolving among parsed types
 * 	- name conversion on defined types
 * 	- and starting generation of TTCN-3 modules
 *
 */
class TTCN3ModuleInventory {
  /**
   * contains all defined TTCN-3 modules
   */
  List<TTCN3Module*> definedModules;

  /**
   * Member to help avoiding to print import twice or more times
   */
  List<TTCN3Module*> writtenImports;

  /**
   * Next three members are used by name conversion part
   * as global variables
   */
  List<QualifiedName> typenames;

  /**
   * Used to register errors during the processing phase
   * After this it is possible to check it
   * and if errors occur possible to stop converter
   */
  static unsigned int num_errors;
  static unsigned int num_warnings;

  TTCN3ModuleInventory(const TTCN3ModuleInventory &); // not implemented
  TTCN3ModuleInventory & operator=(const TTCN3ModuleInventory &); // not implemented
  TTCN3ModuleInventory(); // private due to singleton
  ~TTCN3ModuleInventory(); // private due to singleton
public:

  static TTCN3ModuleInventory& getInstance(); // singleton access

  /**
   * Generation of a new module for the given xsd file
   */
  TTCN3Module * addModule(const char * xsd_filename, XMLParser * a_parser);

  /**
   * Steps after all xsd files are parsed
   */
  void modulenameConversion();
  void referenceResolving();
  void nameConversion();
  void finalModification();

  /**
   * TTCN-3 module generating method
   * Generate TTCN-3 files
   */
  void moduleGeneration();
  /**
   * Prints the generated module names to the output.
   * Used by makefilegen tool.
   */
  void printModuleNames() const;

  List<TTCN3Module*> & getModules() {
    return definedModules;
  }

  List<TTCN3Module*> & getWrittenImports() {
    return writtenImports;
  }

  List<QualifiedName> & getTypenames() {
    return typenames;
  }

  /**
   * Searching methods
   * Look for a simpleType (or element or attribute) or a complexType (or attributeGroup or group)
   */
  RootType * lookup(const RootType * ref, const Mstring& reference, wanted w) const;
  RootType * lookup(const SimpleType * reference, wanted w) const;
  RootType * lookup(const ComplexType * reference, wanted w) const;
  RootType * lookup(const Mstring& name, const Mstring& nsuri,
      const RootType *reference, wanted w) const;

  static unsigned int getNumErrors() {
    return num_errors;
  }

  static unsigned int getNumWarnings() {
    return num_warnings;
  }

  static void incrNumErrors() {
    ++num_errors;
  }

  static void incrNumWarnings() {
    ++num_warnings;
  }

  void dump() const;
};

#endif /* TTCN3MODULEINVENTORY_HH_ */
