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
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szalay, Akos
 *
 ******************************************************************************/
#include "XMLParser.hh"
#include "TTCN3ModuleInventory.hh"

#include "GeneralFunctions.hh"

#include "TTCN3Module.hh"
#include "SimpleType.hh"
#include "ComplexType.hh"
#include "Constant.hh"
#include "converter.hh"

extern bool h_flag_used;
extern bool q_flag_used;



unsigned int TTCN3ModuleInventory::num_errors = 0;
unsigned int TTCN3ModuleInventory::num_warnings = 0;

TTCN3ModuleInventory::TTCN3ModuleInventory()
: definedModules()
, writtenImports()
, typenames() {
}

TTCN3ModuleInventory::~TTCN3ModuleInventory() {
  for (List<TTCN3Module*>::iterator module = definedModules.begin(); module; module = module->Next) {
    delete(module->Data);
  }
}

TTCN3ModuleInventory& TTCN3ModuleInventory::getInstance() {
  // Singleton, see Meyers, More Effective C++, Item 26 (page 131)
  static TTCN3ModuleInventory instance;
  return instance;
}

TTCN3Module * TTCN3ModuleInventory::addModule(const char * xsd_filename, XMLParser * a_parser) {
  TTCN3Module * module = new TTCN3Module(xsd_filename, a_parser);
  definedModules.push_back(module);
  return definedModules.back();
}

void TTCN3ModuleInventory::modulenameConversion() {
  definedModules.sort(compareModules);

  for (List<TTCN3Module*>::iterator module = definedModules.begin(); module; module = module->Next) {
    module->Data->TargetNamespace2ModuleName();
  }
}

void TTCN3ModuleInventory::referenceResolving() {
  /**
   * Reference resolving for include and import statements
   */
  for (List<TTCN3Module*>::iterator module = definedModules.begin(); module; module = module->Next) {
    for (List<RootType*>::iterator type = module->Data->getDefinedTypes().begin(); type; type = type->Next) {
      if (type->Data->getName().convertedValue == "import" || type->Data->getName().convertedValue == "include") {
        type->Data->referenceResolving();
      }
    }
  }

  /**
   * Reference resolving for all others
   */
  bool there_is_unresolved_reference_somewhere = false;
  do {
    there_is_unresolved_reference_somewhere = false;

    for (List<TTCN3Module*>::iterator module = definedModules.begin(); module; module = module->Next) {
      for (List<RootType*>::iterator type = module->Data->getDefinedTypes().begin(); type; type = type->Next) {
        if (type->Data->getName().convertedValue != "import" && type->Data->getName().convertedValue != "include") {
          type->Data->referenceResolving();
          if (type->Data->hasUnresolvedReference()) {
            there_is_unresolved_reference_somewhere = true;
          }
        }
      }
    }
  } while (there_is_unresolved_reference_somewhere);
}

void TTCN3ModuleInventory::finalModification() {
  for (List<TTCN3Module*>::iterator module = definedModules.begin(); module; module = module->Next) {
    for (List<RootType*>::iterator type = module->Data->getDefinedTypes().begin(); type; type = type->Next) {
      type->Data->finalModification();
    }
  }
  for (List<TTCN3Module*>::iterator module = definedModules.begin(); module; module = module->Next) {
    for (List<RootType*>::iterator type = module->Data->getConstantDefs().begin(); type; type = type->Next) {
      type->Data->finalModification();
    }
    Constant::finalFinalModification(module->Data->getConstantDefs());
  }
  for (List<TTCN3Module*>::iterator module = definedModules.begin(); module; module = module->Next) {
    for (List<RootType*>::iterator type = module->Data->getDefinedTypes().begin(); type; type = type->Next) {
      type->Data->finalModification2();
    }
  }
}

void TTCN3ModuleInventory::nameConversion() {
  /**
   * Sort of types and fields
   */

  definedModules.sort(compareModules);
  /********************************************************
   * Conversion of the name of types
   * ******************************************************/
  for (List<TTCN3Module*>::iterator module = definedModules.begin(); module; module = module->Next) {
    if (module->Data->isnotIntoNameConversion()) continue;

    List<RootType*> definedConstants_inABC;
    List<RootType*> definedElements_inABC;
    List<RootType*> definedAttributes_inABC;
    List<RootType*> definedSimpleTypes_inABC;
    List<RootType*> definedComplexTypes_inABC;
    List<RootType*> definedAttributeGroups_inABC;
    List<RootType*> definedGroups_inABC;

    for (List<TTCN3Module*>::iterator module2 = module; module2; module2 = module2->Next) {
      if (module2->Data->getModulename() != module->Data->getModulename()) continue;
      
      for (List<RootType*>::iterator type = module2->Data->getConstantDefs().begin(); type; type = type->Next) {
        definedConstants_inABC.push_back(type->Data);
      }
      // If two module has the same module name then they will be generated into
      // one module, so the constants shall be moved.
      if (module->Data != module2->Data) {
        for (List<RootType*>::iterator type = module->Data->getConstantDefs().begin(), nextType; type; type = nextType) {
          nextType = type->Next;
          module2->Data->getConstantDefs().push_back(type->Data);
          module->Data->getConstantDefs().remove(type);
        }
      }

      for (List<RootType*>::iterator type = module2->Data->getDefinedTypes().begin(); type; type = type->Next) {
        switch (type->Data->getConstruct()) {
          case c_simpleType:
            definedSimpleTypes_inABC.push_back(type->Data);
            break;
          case c_element:
            definedElements_inABC.push_back(type->Data);
            break;
          case c_attribute:
            definedAttributes_inABC.push_back(type->Data);
            break;
          case c_complexType:
            definedComplexTypes_inABC.push_back(type->Data);
            break;
          case c_group:
            definedGroups_inABC.push_back(type->Data);
            break;
          case c_attributeGroup:
            definedAttributeGroups_inABC.push_back(type->Data);
            break;
          default:
            break;
        }
      }
      module2->Data->notIntoNameConversion();
    }

    definedSimpleTypes_inABC.sort(compareTypes);
    definedElements_inABC.sort(compareTypes);
    definedAttributes_inABC.sort(compareTypes);
    definedSimpleTypes_inABC.sort(compareTypes);
    definedComplexTypes_inABC.sort(compareTypes);
    definedAttributeGroups_inABC.sort(compareTypes);
    definedGroups_inABC.sort(compareTypes);

    typenames.push_back(QualifiedName(module->Data->getTargetNamespace(), module->Data->getModulename()));
    for(List<const TTCN3Module*>::iterator mod = module->Data->getImportedModules().begin(); mod; mod = mod->Next){
      typenames.push_back(QualifiedName(module->Data->getTargetNamespace(), mod->Data->getModulename()));
    }

    for (List<RootType*>::iterator type = definedConstants_inABC.begin(); type; type = type->Next) {
      type->Data->nameConversion(nameMode, module->Data->getDeclaredNamespaces());
    }
    for (List<RootType*>::iterator type = definedElements_inABC.begin(); type; type = type->Next) {
      type->Data->nameConversion(nameMode, module->Data->getDeclaredNamespaces());
    }
    for (List<RootType*>::iterator type = definedAttributes_inABC.begin(); type; type = type->Next) {
      type->Data->nameConversion(nameMode, module->Data->getDeclaredNamespaces());
    }
    for (List<RootType*>::iterator type = definedSimpleTypes_inABC.begin(); type; type = type->Next) {
      type->Data->nameConversion(nameMode, module->Data->getDeclaredNamespaces());
    }
    for (List<RootType*>::iterator type = definedComplexTypes_inABC.begin(); type; type = type->Next) {
      type->Data->nameConversion(nameMode, module->Data->getDeclaredNamespaces());
    }
    for (List<RootType*>::iterator type = definedAttributeGroups_inABC.begin(); type; type = type->Next) {
      type->Data->nameConversion(nameMode, module->Data->getDeclaredNamespaces());
    }
    for (List<RootType*>::iterator type = definedGroups_inABC.begin(); type; type = type->Next) {
      type->Data->nameConversion(nameMode, module->Data->getDeclaredNamespaces());
    }
    typenames.clear();
  }
  /********************************************************
   * Conversion of the type of types
   * ******************************************************/
  for (List<TTCN3Module*>::iterator module = definedModules.begin(); module; module = module->Next) {
    for (List<RootType*>::iterator type = module->Data->getConstantDefs().begin(); type; type = type->Next) {
      type->Data->nameConversion(typeMode, module->Data->getDeclaredNamespaces());
    }
    for (List<RootType*>::iterator type = module->Data->getDefinedTypes().begin(); type; type = type->Next) {
      type->Data->nameConversion(typeMode, module->Data->getDeclaredNamespaces());
    }
  }
  /********************************************************
   * Conversion of the names and the types of the fields
   * ******************************************************/
  for (List<TTCN3Module*>::iterator module = definedModules.begin(); module; module = module->Next) {
    for (List<RootType*>::iterator type = module->Data->getDefinedTypes().begin(); type; type = type->Next) {
      type->Data->nameConversion(fieldMode, module->Data->getDeclaredNamespaces());
    }
  }
}

void TTCN3ModuleInventory::moduleGeneration() {
  for (List<TTCN3Module*>::iterator module = definedModules.begin(); module; module = module->Next) {
    if (module->Data->isnotIntoFile()) {
      continue;
    }

    List<NamespaceType> used_namespaces;
    NamespaceType targetns;
    targetns.uri = module->Data->getTargetNamespace();
    used_namespaces.push_back(targetns);

    // Now search for other modules with the same module name.
    // They must have had the same targetNamespace.
    for (List<TTCN3Module*>::iterator module2 = module; module2; module2 = module2->Next) {
      if (module2->Data->getModulename() != module->Data->getModulename()) {
        continue;
      }

      for (List<NamespaceType>::iterator declNS = module2->Data->getDeclaredNamespaces().begin(); declNS; declNS = declNS->Next) {
        used_namespaces.push_back(declNS->Data);
      }
      module2->Data->notIntoFile(); // first module gets the TTCN-3 file
    }

    Mstring filename_s = module->Data->getModulename() + ".ttcn";
    FILE * file = fopen(filename_s.c_str(), "w");
    if (file == NULL) {
      Mstring cannot_write("Cannot write file ");
      perror((cannot_write + filename_s).c_str());
      ++num_errors;
      return;
    }
#ifndef NDEBUG
    // In debug mode, set the output stream to unbuffered.
    // This allows watching the output as it appears (e.g. with tail -f).
    setvbuf(file, NULL, _IONBF, 0);
#endif

    generate_TTCN3_header(file, module->Data->getModulename().c_str());

    fprintf(file, "//\tGenerated from file(s):\n");

    for (List<TTCN3Module*>::iterator module2 = module; module2; module2 = module2->Next) {
      if (module2->Data->getModulename() == module->Data->getModulename()) {
        module2->Data->generate_TTCN3_fileinfo(file);
      }
    }

    fprintf(file,
      "////////////////////////////////////////////////////////////////////////////////\n"
      "//     Modification header(s):\n"
      "//-----------------------------------------------------------------------------\n"
      "//  Modified by:\n"
      "//  Modification date:\n"
      "//  Description:\n"
      "//  Modification contact:\n"
      "//------------------------------------------------------------------------------\n"
      "////////////////////////////////////////////////////////////////////////////////\n"
      "\n"
      "\n");

    module->Data->generate_TTCN3_modulestart(file);

    for (List<TTCN3Module*>::iterator module2 = module; module2; module2 = module2->Next) {
      if (module2->Data->getModulename() == module->Data->getModulename()) {
        module2->Data->generate_TTCN3_import_statements(file);
      }
    }

    writtenImports.clear();

    for (List<TTCN3Module*>::iterator module2 = module; module2; module2 = module2->Next) {
      if (module2->Data->getModulename() == module->Data->getModulename()) {
        module2->Data->generate_TTCN3_included_types(file);
        module2->Data->generate_TTCN3_types(file);
      }
    }

    fprintf(file, "}\n");

    module->Data->generate_with_statement(file, used_namespaces);

    if (!q_flag_used) fprintf(stderr, "Notify: File '%s' was generated.\n", filename_s.c_str());

    fclose(file);
  }
}

void TTCN3ModuleInventory::printModuleNames() const {
  for (List<TTCN3Module*>::iterator module = definedModules.begin(); module; module = module->Next) {
    if (module->Data->getModulename() == XMLSchema) continue;
    if (module->Data->isnotIntoFile()) continue;
    
    fprintf(stdout, "%s", module->Data->getModulename().c_str());
    // Now search for other modules with the same module name.
    // They must have had the same targetNamespace.
    for (List<TTCN3Module*>::iterator module2 = module; module2; module2 = module2->Next) {
      if (module2->Data->getModulename() != module->Data->getModulename()) {
        continue;
      }
      module2->Data->notIntoFile();
    }
  }
}

RootType * TTCN3ModuleInventory::lookup(const RootType* ref, const Mstring& reference, wanted w) const {
  Mstring uri = reference.getPrefix(':');
  const Mstring& name = reference.getValueWithoutPrefix(':');
  if(uri.empty()){
    for(List<NamespaceType>::iterator qname = ref->getModule()->getDeclaredNamespaces().begin(); qname; qname = qname->Next){
      if(qname->Data.prefix.empty()){
        uri = qname->Data.uri;
        break;
      }
    }
    if(uri.empty()){
      //If the targetnamespace is NoTargetNamespace therefore no prefix connector used
      uri = ref->getModule()->getTargetNamespace();
    }
  }else {
    uri = getNameSpaceByPrefix(ref, uri);
  }
  return lookup(name, uri, NULL, w);
}

RootType * TTCN3ModuleInventory::lookup(const SimpleType * reference, wanted w) const {
  const Mstring& uri = reference->getReference().get_uri();
  const Mstring& name = reference->getReference().get_val();

  return lookup(name, uri, reference, w);
}

RootType * TTCN3ModuleInventory::lookup(const ComplexType * reference, wanted w) const {
  const Mstring& uri = reference->getReference().get_uri();
  const Mstring& name = reference->getReference().get_val();

  return lookup(name, uri, reference, w);
}

RootType * TTCN3ModuleInventory::lookup(const Mstring& name, const Mstring& nsuri,
  const RootType *reference, wanted w) const {
  return ::lookup(definedModules, name, nsuri, reference, w);
}

void TTCN3ModuleInventory::dump() const {
  fprintf(stderr, "Dumping %lu modules.\n", static_cast<unsigned long>( definedModules.size() ));
  for (List<TTCN3Module*>::iterator module = definedModules.begin(); module; module = module->Next) {
    module->Data->dump();
  }

  fprintf(stderr, "Dumping %lu types\n", static_cast<unsigned long>( typenames.size() ));

  Item<QualifiedName> *o = typenames.begin();
  for (; o != NULL; o = o->Next) {
    fprintf(stderr, "{%s}%s,\n",
      o->Data.nsuri.c_str(), o->Data.name.c_str());
  }
}
