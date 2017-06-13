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
 *   Lovassy, Arpad
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szalay, Akos
 *
 ******************************************************************************/
#include "TTCN3Module.hh"

#include "RootType.hh"
#include "SimpleType.hh"
#include "ComplexType.hh"
#include "ImportStatement.hh"
#include "Annotation.hh"
#include "Constant.hh"
#include "converter.hh"

#include "../common/version_internal.h"

#include <cctype>

#if defined(WIN32) && !defined(MINGW)
#include <cygwin/version.h>
#include <sys/cygwin.h>
#include <limits.h>
#endif

extern bool e_flag_used;
extern bool z_flag_used;

TTCN3Module::TTCN3Module(const char * a_filename, XMLParser * a_parser)
: parser(a_parser)
, schemaname()
, modulename()
, definedTypes()
, constantDefs()
, actualXsdConstruct(c_unknown)
, xsdVersion()
, xsdEncoding()
, xsdStandalone()
, targetNamespace()
, targetNamespace_connectedPrefix()
, declaredNamespaces()
, elementFormDefault(notset)
, attributeFormDefault(notset)
, blockDefault(not_set)
, storedTypeSubstitutions()
, element_types()
//, importedModules()
, variant()
, xmlSchemaPrefixes()
, moduleNotIntoFile(false)
, moduleNotIntoNameConversion(false)
, const_counter(1) {
#if defined(WIN32) && !defined(MINGW)
  // Transform a Windows style path: "C:\cygwin\tmp\a.xsd"
  // into a POSIX style path: "/home/a/xsd", so getValueWithoutPrefix('/')
  // can chop off the directory path.
#if CYGWIN_VERSION_DLL_MAJOR >= 1007
  char *posix = NULL;
  ssize_t needed = cygwin_conv_path(CCP_WIN_A_TO_POSIX | CCP_RELATIVE, a_filename, NULL, 0);
  if (needed >= 0) {
    posix = static_cast<char*>( Malloc(needed) );
    if (cygwin_conv_path(CCP_WIN_A_TO_POSIX | CCP_RELATIVE, a_filename, posix, needed)) {
      posix = NULL; // conversion failed
    }
  }
  Mstring filename(posix ? posix : a_filename);
  Free(posix); // even if NULL
#else // Cygwin 1.5
  char posix[PATH_MAX];
  int fail = cygwin_conv_to_posix_path(a_filename, posix);
  Mstring filename(fail ? a_filename : posix);
#endif

#else
  Mstring filename(a_filename);
#endif
  schemaname = filename.getValueWithoutPrefix('/'); // excludes the path of the input file
}

TTCN3Module::~TTCN3Module() {
  for (List<RootType*>::iterator type = definedTypes.begin(); type; type = type->Next) {
    delete type->Data;
  }
  
  for (List<RootType*>::iterator type = constantDefs.begin(); type; type = type->Next) {
    delete type->Data;
  }
}

void TTCN3Module::loadValuesFromXMLDeclaration(const char *a_version,
  const char *a_encoding, int a_standalone) {
  xsdVersion = a_version;
  xsdEncoding = a_encoding;
  xsdStandalone = a_standalone;
}

void TTCN3Module::loadValuesFromSchemaTag(const Mstring& a_targetNamespace,
  List<NamespaceType> a_declaredNamespaces,
  FormValue a_elementFormDefault, FormValue a_attributeFormDefault,
  BlockValue a_blockDefault) {
  if (a_targetNamespace.empty()) {
    targetNamespace = "NoTargetNamespace";
  } else {
    if (a_targetNamespace == XMLSchema) {
      notIntoFile();
    }
    targetNamespace = a_targetNamespace;
  }

  elementFormDefault = a_elementFormDefault;
  attributeFormDefault = a_attributeFormDefault;
  blockDefault = a_blockDefault;

  declaredNamespaces = a_declaredNamespaces;

  for (List<NamespaceType>::iterator ns = declaredNamespaces.begin(); ns; ns = ns->Next) {
    if (ns->Data.uri == targetNamespace) {
      targetNamespace_connectedPrefix = ns->Data.prefix;
      break;
    }
  }
  
  for (List<NamespaceType>::iterator ns = declaredNamespaces.begin(); ns; ns = ns->Next) {
    if (ns->Data.uri == XMLSchema) {
      Mstring prefix = ns->Data.prefix;
      prefix.removeWSfromBegin();
      prefix.removeWSfromEnd();
      xmlSchemaPrefixes.push_back(prefix);
    }
  }
}

void TTCN3Module::addMainType(const ConstructType typeOfMainType) {
  switch (typeOfMainType) {
    case c_simpleType:
    {
      SimpleType * new_ST = new SimpleType(parser, this, c_simpleType);
      definedTypes.push_back(new_ST);
      new_ST->loadWithValues();
      break;
    }
    case c_element:
    {
      SimpleType * new_ST = new SimpleType(parser, this, c_element);
      definedTypes.push_back(new_ST);
      new_ST->loadWithValues();
      break;
    }
    case c_attribute:
    {
      SimpleType * new_ST = new SimpleType(parser, this, c_attribute);
      definedTypes.push_back(new_ST);
      new_ST->loadWithValues();
      break;
    }
    case c_complexType:
    {
      ComplexType * new_CT = new ComplexType(parser, this, c_complexType);
      definedTypes.push_back(new_CT);
      new_CT->loadWithValues();
      break;
    }
    case c_group:
    {
      ComplexType * new_CT = new ComplexType(parser, this, c_group);
      definedTypes.push_back(new_CT);
      new_CT->loadWithValues();
      break;
    }
    case c_attributeGroup:
    {
      ComplexType * new_CT = new ComplexType(parser, this, c_attributeGroup);
      definedTypes.push_back(new_CT);
      new_CT->loadWithValues();
      break;
    }
    case c_include:
    {
      ImportStatement * new_INCL = new ImportStatement(parser, this, c_include);
      definedTypes.push_back(new_INCL);
      new_INCL->loadWithValues();
      break;
    }
    case c_import:
    {
      ImportStatement * new_IMP = new ImportStatement(parser, this, c_import);
      definedTypes.push_back(new_IMP);
      new_IMP->loadWithValues();
      break;
    }
    case c_annotation:
    {
      Annotation * new_ANN = new Annotation(parser, this, c_annotation);
      definedTypes.push_back(new_ANN);
      new_ANN->loadWithValues();
      break;
    }
    case c_idattrib:
    {
      Mstring type = empty_string;
      if (hasDefinedMainType()) {
        type = getLastMainType().getName().convertedValue;
      }
      printWarning(getSchemaname(), type,
        Mstring("The mapping of ID attribute is not supported."));
      TTCN3ModuleInventory::getInstance().incrNumWarnings();
      break;
    }
    case c_unknown:
    case c_schema:
      break;
  }

  actualXsdConstruct = typeOfMainType;
}

void TTCN3Module::replaceLastMainType(RootType * t) {
  delete(definedTypes.back());
  definedTypes.pop_back();
  definedTypes.push_back(t);
  actualXsdConstruct = t->getConstruct();
}

void TTCN3Module::generate_TTCN3_fileinfo(FILE * file) {
  fprintf(file,
    "//\t- %s\n"
    "//\t\t\t/* xml ",
    schemaname.c_str()
    );

  if (!xsdVersion.empty()) {
    fprintf(file, "version = \"%s\" ", xsdVersion.c_str());
  }
  if (!xsdEncoding.empty()) {
    fprintf(file, "encoding = \"%s\" ", xsdEncoding.c_str());
  }

  switch (xsdStandalone) {
    case 0:
      fprintf(file, "standalone = \"no\" ");
      break;
    case 1:
      fprintf(file, "standalone = \"yes\" ");
      break;
    default:
      break;
  }

  fprintf(file,
    "*/\n"
    "//\t\t\t/* targetnamespace = \"%s\" */\n",
    targetNamespace.c_str()
    );
}

void TTCN3Module::generate_TTCN3_modulestart(FILE * file) {
  fprintf(file,
    "module %s {\n"
    "\n"
    "\n"
    "import from XSD all;\n"
    "\n"
    "\n",
    modulename.c_str()
    );
}

void TTCN3Module::generate_TTCN3_import_statements(FILE * file) {
  for (List<RootType*>::iterator type = definedTypes.begin(); type; type = type->Next) {
    if (type->Data->getConstruct() == c_import) {
      type->Data->printToFile(file);
    }
  }
}

void TTCN3Module::generate_TTCN3_included_types(FILE * file) {
  for (List<RootType*>::iterator type = definedTypes.begin(); type; type = type->Next) {
    if (type->Data->getConstruct() == c_include) {
      type->Data->printToFile(file);
    }
  }
}

void TTCN3Module::generate_TTCN3_types(FILE * file) {
  for (List<RootType*>::iterator type = constantDefs.begin(); type; type = type->Next) {
    type->Data->printToFile(file);
  }
  for (List<RootType*>::iterator type = definedTypes.begin(); type; type = type->Next) {
    if (type->Data->getConstruct() != c_include && type->Data->getConstruct() != c_import) {
      type->Data->printToFile(file);
    }
  }
}

void TTCN3Module::generate_with_statement(FILE * file, List<NamespaceType> used_namespaces) {
  if (e_flag_used) {
    return;
  }

  fprintf(file,
    "with {\n"
    "  encode \"XML\";\n"
    );

  bool xsi = false;

  for (List<NamespaceType>::iterator usedNS = used_namespaces.begin(); usedNS; usedNS = usedNS->Next) {
    if (usedNS->Data.uri == XMLSchema) {
      xsi = true;
      continue;
    }
    if (usedNS->Data.uri == "NoTargetNamespace") {
      continue;
    }
    // XXX this inner loop is either redundant now, or it should be elsewhere.
    // It is quite dodgy to modify(!) namespaces when we are already generating code.
    for (List<NamespaceType>::iterator usedNS2 = usedNS->Next; usedNS2; usedNS2 = usedNS2->Next) {
      if (usedNS->Data.uri == usedNS2->Data.uri) {
        if (usedNS2->Data.prefix.empty())
          usedNS2->Data.prefix = usedNS->Data.prefix;
        break;
      }
    }
  }

  if (targetNamespace != "NoTargetNamespace") {
    fprintf(file, "  variant \"namespace as \'%s\'", targetNamespace.c_str());
    if (!targetNamespace_connectedPrefix.empty()) {
      fprintf(file, " prefix \'%s\'", targetNamespace_connectedPrefix.c_str());
    }
    fprintf(file, "\";\n");
  }


  if (xsi) {
    fprintf(file,
      "  variant \"controlNamespace \'%s-instance\' prefix \'xsi\'\";\n", XMLSchema);
  }
  if (attributeFormDefault == qualified) {
    fprintf(file,
      "  variant \"attributeFormQualified\";\n");
  }
  if (elementFormDefault == qualified) {
    fprintf(file,
      "  variant \"elementFormQualified\";\n");
  }
  fprintf(file,
    "}\n");
}

void TTCN3Module::TargetNamespace2ModuleName() {
  Mstring res(targetNamespace);

  if (z_flag_used) {
    char * found;
    found = res.foundAt("http://");
    //check if the http:// is at the beginning of the namespace
    if (found == res.c_str()) { //res.c_str() returns a pointer to the first char
      for (int i = 0; i != 7; ++i) {
        res.eraseChar(0);
      }
    }
    found = res.foundAt("urn:");
    //check if the urn: is at the beginning of the namespace
    if (found == res.c_str()) { //res.c_str() returns a pointer to the first char
      for (int i = 0; i != 4; ++i) {
        res.eraseChar(0);
      }
    }
  }

  // the characters ' '(SPACE), '.'(FULL STOP) and '-'(HYPEN-MINUS)
  // and '/', '#', ':' shall all be replaced by a "_" (LOW LINE)
  for (size_t i = 0; i != res.size(); ++i) {
    if ((res[i] == ' ') ||
      (res[i] == '.') ||
      (res[i] == '-') ||
      (res[i] == '/') ||
      (res[i] == '#') ||
      (res[i] == ':')) {
      res[i] = '_';
    }
  }
  // any character except "A" to "Z", "a" to "z" or "0" to "9" and "_" shall be removed
  for (size_t i = 0; i != res.size(); ++i) {
    if (!isalpha(static_cast<const unsigned char>( res[i] )) && !isdigit(static_cast<const unsigned char>( res[i] )) && (res[i] != '_')) {
      res.eraseChar(i);
      i--;
    }
  }
  // a sequence of two of more "_" (LOW LINE) shall be replaced with a single "_" (LOW LINE)
  for (size_t i = 1; i < res.size(); ++i) {
    if (res[i] == '_' && res[i - 1] == '_') {
      res.eraseChar(i);
      i--;
    }
  }

  if (!res.empty()) {
    // "_" (LOW LINE) characters occurring at the beginning of the name shall be removed
    if (res[0] == '_') {
      res.eraseChar(0);
    }
  }
  if (!res.empty()) {
    // "_" (LOW LINE) characters occurring at the end of the name shall be removed
    if (res[res.size() - 1] == '_') {
      res.eraseChar(res.size() - 1);
    }
  }

  if (res.empty()) {
    res = "x";
  } else if (isdigit(static_cast<const unsigned char>( res[0] ))) {
    res.insertChar(0, 'x');
  }

  //Postfix with _i if the targetnamespace is different
  bool postfixing = false;
  for (List<TTCN3Module*>::iterator mod = TTCN3ModuleInventory::getInstance().getModules().begin(); mod; mod = mod->Next) {
    if (mod->Data != this && mod->Data->getModulename() == res && mod->Data->getTargetNamespace() != targetNamespace) {
      postfixing = true;
      break;
    }
  }

  if(postfixing){
    bool found;
    int counter = 1;
    expstring_t tmpname = NULL;
    do {
      found = false;
      Free(tmpname);
      tmpname = mprintf("%s_%d", res.c_str(), counter);
      for(List<TTCN3Module*>::iterator mod = TTCN3ModuleInventory::getInstance().getModules().begin(); mod; mod = mod->Next){
        if(mod->Data != this && mod->Data->getModulename() == Mstring(tmpname)){
          found = true;
          break;
        }
      }
      counter++;
    } while (found);
    res = Mstring(tmpname);
    Free(tmpname);
  }

  modulename = res;
}

void TTCN3Module::dump() const {
  fprintf(stderr, "Module '%s' at %p (from %s)\n",
    modulename.c_str(), static_cast<const void*>( this ), schemaname.c_str());
  
  for (List<RootType*>::iterator type = constantDefs.begin(); type; type = type->Next) {
    type->Data->dump(1);
  }

  for (List<RootType*>::iterator type = definedTypes.begin(); type; type = type->Next) {
    type->Data->dump(1);
  }
}
