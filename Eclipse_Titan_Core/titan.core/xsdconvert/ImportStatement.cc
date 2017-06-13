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
#include "ImportStatement.hh"

#include "GeneralFunctions.hh"
#include "TTCN3Module.hh"
#include "TTCN3ModuleInventory.hh"

ImportStatement::ImportStatement(XMLParser * a_parser, TTCN3Module * a_module, ConstructType a_construct)
: RootType(a_parser, a_module, a_construct)
, from_namespace()
, from_schemaLocation()
, source_module() {
}

void ImportStatement::loadWithValues() {
  const XMLParser::TagAttributes & attr = parser->getActualTagAttributes();

  switch (parser->getActualTagName()) {
    case n_import:
      name.upload(Mstring("import"));
      type.upload(Mstring("import"), false);
      from_namespace = attr.namespace_;
      from_schemaLocation = attr.schemaLocation;
      break;
    case n_include:
      name.upload(Mstring("include"));
      type.upload(Mstring("include"), false);
      from_namespace = attr.namespace_;
      from_schemaLocation = attr.schemaLocation;
      break;
    case n_label:
      addComment(Mstring("LABEL:"));
      break;
    case n_definition:
      addComment(Mstring("DEFINITION:"));
      break;

    default:
      break;
  }
}

void ImportStatement::referenceResolving(void) {
  if (from_namespace == XMLSchema) {
    visible = false;
    return;
  }

  TTCN3ModuleInventory& modules = TTCN3ModuleInventory::getInstance();

  for (List<TTCN3Module*>::iterator mod = modules.getModules().begin(); mod; mod = mod->Next) {
    if (module == mod->Data) {
      // it's the module that *contains* the import statement
      continue;
    }
    // Try the namespace first
    if (from_namespace == mod->Data->getTargetNamespace()) {
      source_module = mod->Data;
      break;
    }
    // Fallback: try the schemaLocation attribute
    if (!from_schemaLocation.empty()) {
      if (from_schemaLocation == mod->Data->getSchemaname()) {
        source_module = mod->Data;
        from_namespace = mod->Data->getTargetNamespace();
        // do not break; give a chance to other modules to match the namespace
      }
    }
  }

  if (!source_module) // still not found
  {
    if (from_schemaLocation.empty()) {
      printWarning(module->getSchemaname(), getName().convertedValue,
        "The \'" + from_namespace + "\' specified in the \'namespace\' attribute"
        " is not resolvable.");
      modules.incrNumWarnings();
    } else // schemaLocation is not empty
    {
      if (from_schemaLocation.isFound("http://") || from_schemaLocation.isFound("urn:")) {
        printWarning(module->getSchemaname(), getName().convertedValue,
          "It is not supported using a URI (\'" + from_schemaLocation +
          "\') in the \'schemalocation\' attribute to get access to a file.");
        modules.incrNumWarnings();
      } else {
        printWarning(module->getSchemaname(), getName().convertedValue,
          "The \'" + from_schemaLocation + "\' specified in the \'schemaLocation\' attribute"
          " is not resolvable.");
        modules.incrNumWarnings();
      }
    }
    visible = false;
  } else module->addImportedModule(source_module);
}

void ImportStatement::printToFile(FILE * file) {
  if (!visible) return;

  if (from_namespace == module->getTargetNamespace()) return;
  // Not include or import in this case: including is automatic because modules have the same targetnamespace

  printComment(file);

  switch (getConstruct()) {
    case c_import:
    {
      bool found = false;
      for (List<TTCN3Module*>::iterator wImport = TTCN3ModuleInventory::getInstance().getWrittenImports().begin(); wImport; wImport = wImport->Next) {
        if (wImport->Data == source_module) {
          found = true;
          break;
        }
      }
      if (!found) {
        fprintf(file, "import from %s all;\n\n\n", source_module->getModulename().c_str());
        TTCN3ModuleInventory::getInstance().getWrittenImports().push_back(source_module);
      }
      break;
    }
    case c_include:
    {
      for (List<TTCN3Module*>::iterator mod = TTCN3ModuleInventory::getInstance().getModules().begin(); mod; mod = mod->Next) {
        if (mod->Data->getSchemaname() == from_schemaLocation) {
          mod->Data->generate_TTCN3_types(file);
          break;
        }
      }
      break;
    }
    default:
      break;
  }
}

void ImportStatement::dump(unsigned int depth) const {
  fprintf(stderr, "%*s Import statement at %p, ns='%s' loc='%s'\n", depth * 2, "",
    (const void*) this, from_namespace.c_str(), from_schemaLocation.c_str());
  fprintf(stderr, "%*s import from %s into %s\n", depth * 2 + 2, "",
    (source_module ? source_module->getModulename().c_str() : "**unknown**"),
    module->getModulename().c_str());
}

