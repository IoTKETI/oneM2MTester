/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
#include "AttributeType.hh"

AttributeType::AttributeType(ComplexType * a_complexType)
: SimpleType(a_complexType->getParser(), a_complexType->getModule(), c_unknown)
, isAnyAttr(false)
, useVal(optional)
, nameSpace(empty_string)
, used(false)
, origModule(a_complexType->getModule()) {
  parent = a_complexType;
}

AttributeType::AttributeType(const AttributeType & other)
: SimpleType(other)
, isAnyAttr(other.isAnyAttr)
, useVal(other.useVal)
, nameSpace(other.nameSpace)
, used(other.used)
, origModule(other.getModule()) {
}

void AttributeType::modifyValues() {
  if (parser->getActualTagName() == n_attribute || parser->getActualTagName() == n_anyAttribute) {
    ((ComplexType*) parent)->modifyAttributeParent();
  }
}

void AttributeType::setTypeOfField(const Mstring& in) {
  type.upload(in);
}

void AttributeType::setNameOfField(const Mstring& in) {
  name.upload(in);
}

void AttributeType::setToAnyAttribute() {
  isAnyAttr = true;
}

void AttributeType::setFieldPath(const Mstring path) {
  if (path.empty()) {
    actualPath = getName().convertedValue;
  } else {
    if ((parent->getMinOccurs() != 1 || parent->getMaxOccurs() != 1) && parent->getName().list_extension) {
      actualPath = path + Mstring("[-].") + getName().convertedValue;
    } else {
      actualPath = path + Mstring(".") + getName().convertedValue;
    }
  }
}

void AttributeType::collectVariants(List<Mstring>& container) {

  if (variant.empty() && hidden_variant.empty()) {
    return;
  }

  if (!isVisible()) {
    return;
  }
  
  enumeration.insertVariants();

  for (List<Mstring>::iterator var2 = variant.end(); var2; var2 = var2->Prev) {
    container.push_back(Mstring("variant (") + actualPath + Mstring(") ") + Mstring(var2->Data.c_str()) + Mstring(";\n"));
  }
  for (List<Mstring>::iterator hidden_var = hidden_variant.end(); hidden_var; hidden_var = hidden_var->Prev) {
    container.push_back(Mstring("//variant (") + actualPath + Mstring(") ") + Mstring(hidden_var->Data.c_str()) + Mstring(";\n"));
  }
}

void AttributeType::nameConversion_names(QualifiedNames& used_ns) {
  //Do not convert invisible field names
  if (!visible || useVal == prohibited) {
    return;
  }
  Mstring res, var(module->getTargetNamespace());
  QualifiedNames used_names = TTCN3ModuleInventory::getInstance().getTypenames();
  for (QualifiedNames::iterator n = used_ns.begin(); n; n = n->Next) {
    used_names.push_back(n->Data);
  }
  QualifiedName q;
  if(!used_names.empty()){
    q = used_names.back();
  }else {
    q = QualifiedName(empty_string, empty_string);
  }
  XSDName2TTCN3Name(name.convertedValue, used_names, field_name, res, var);
  name.convertedValue = res;
  addVariant(V_onlyValue, var);
  if (q.name != used_names.back().name) {
    //If the name is converted then push to the used names list
    used_ns.push_back(used_names.back());
  }

  for (List<RootType*>::iterator st = nameDepList.begin(); st; st = st->Next) {
    st->Data->setTypeValue(res);
  }
}

void AttributeType::applyUseAttribute() {
  if (isAnyAttr) {
    return;
  }
  switch (useVal) {
    case optional:
      setMinOccurs(0);
      setMaxOccurs(1);
      break;
    case required:
      setMinOccurs(1);
      setMaxOccurs(1);
      break;
    case prohibited:
      setMinOccurs(0);
      setMaxOccurs(0);
      setInvisible();
      break;
  }
  isOptional = isOptional || (getMinOccurs() == 0 && getMaxOccurs() == 1);
}

void AttributeType::applyNamespaceAttribute(VariantMode varLabel) {
  List<Mstring> namespaces;
  if (!nameSpace.empty()) {
    expstring_t valueToSplitIntoTokens = mcopystr(nameSpace.c_str());
    char * token;
    token = strtok(valueToSplitIntoTokens, " ");
    while (token != NULL) {
      namespaces.push_back(Mstring(token));
      token = strtok(NULL, " ");
    }
    Free(valueToSplitIntoTokens);
  }

  Mstring any_ns;
  bool first = true;
  // Note: libxml2 will verify the namespace list according to the schema
  // of XML Schema. It is either ##any, ##other, ##local, ##targetNamespace,
  // or a list of (namespace reference | ##local | ##targetNamespace).
  for (List<Mstring>::iterator ns = namespaces.begin(); ns; ns = ns->Next) {
    static const Mstring xxany("##any"), xxother("##other"), xxlocal("##local"),
      xxtargetNamespace("##targetNamespace");
    if (!first) any_ns += ", ";

    if (ns->Data == xxany) {
    }// this must be the only element, nothing to add
    else if (ns->Data == xxother) { // this must be the only element
      if(first){ any_ns += " except "; }
      any_ns += "unqualified";
      if (module->getTargetNamespace() != "NoTargetNamespace") {
        any_ns += ", \'";
        any_ns += parent->getModule()->getTargetNamespace();
        any_ns += '\'';
      }
    }// The three cases below can happen multiple times
    else {
      if (first) any_ns += " from ";
      // else a comma was already added
      if (ns->Data == xxtargetNamespace) {
        any_ns += '\'';
        any_ns += parent->getModule()->getTargetNamespace();
        any_ns += '\'';
      } else if (ns->Data == xxlocal) {
        any_ns += "unqualified";
      } else {
        any_ns += '\'';
        any_ns += ns->Data;
        any_ns += '\'';
      }
    }
    if(!first || ns->Data != xxany){
      first = false;
    }
  }

  addVariant(varLabel, any_ns, true);
}

void AttributeType::applyMinMaxOccursAttribute(unsigned long long min, unsigned long long max) {
  setMinOccurs(min);
  setMaxOccurs(max);
}

void AttributeType::dump(unsigned int depth) const {
  fprintf(stderr, "%*s %sField '%s' -> '%s' at %p\n", depth * 2, "", isVisible() ? "" : "(hidden)",
    name.originalValueWoPrefix.c_str(), name.convertedValue.c_str(), (const void*) this);
  fprintf(stderr, "%*s %s Type: \n", depth * 2, "", type.convertedValue.c_str());
  fprintf(stderr, "%*s type %s \n", (depth + 1) * 2, "", type.convertedValue.c_str());
  fprintf(stderr, "%*s (%llu .. %llu)\n", (depth + 1) * 2, "", getMinOccurs(), getMaxOccurs());
  fprintf(stderr, "%*s %d variants: ", (depth + 1) * 2, "", (int) variant.size());
  for (List<Mstring>::iterator var = variant.begin(); var; var = var->Next) {
    fprintf(stderr, "%s, ", var->Data.c_str());
  }
  fprintf(stderr, "\n%*s path =/%s/", (depth + 1) * 2, "", actualPath.c_str());
}

void AttributeType::printToFile(FILE* file, unsigned level) {
  if (!isVisible()) {
    return;
  }
  printComment(file, level);
  indent(file, level);
  if(enumeration.modified && hasVariant(Mstring("\"list\""))){
    printMinOccursMaxOccurs(file, false);
    fprintf(file, "enumerated {\n");
    enumeration.sortFacets();
    enumeration.printToFile(file);
    indent(file, level);
    fprintf(file, "\n} %s", name.convertedValue.c_str());
  } else if (enumeration.modified) {
    if (isFloatType(builtInBase)) {
      fprintf(file, "%s %s (", type.convertedValue.c_str(), name.convertedValue.c_str());
      enumeration.sortFacets();
      enumeration.printToFile(file);
      fputc(')', file);
    } else {
      fprintf(file, "enumerated {\n");
      enumeration.sortFacets();
      enumeration.printToFile(file);
      fprintf(file, "\n");
      indent(file, level);
      fprintf(file, "} %s", name.convertedValue.c_str());
    }
  }else { 
    printMinOccursMaxOccurs(file, false);
    int multiplicity = multi(module, getReference(), this);
    if ((multiplicity > 1) && getReference().get_ref()) {
      fprintf(file, "%s.", getReference().get_ref()->getModule()->getModulename().c_str());
    }
    fprintf(file, "%s %s", type.convertedValue.c_str(), name.convertedValue.c_str());
    getPattern().printToFile(file);
    getValue().printToFile(file);
    getLength().printToFile(file);
  }
    if (isOptional || isAnyAttr) {
    fprintf(file, " optional");
  }
}
