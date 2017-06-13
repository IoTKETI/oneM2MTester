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
#ifndef ATTRIBUTETYPE_HH_
#define ATTRIBUTETYPE_HH_

#include "SimpleType.hh"
#include "GeneralTypes.hh"
#include "GeneralFunctions.hh"
#include "GeneralFunctions.hh"
#include "TTCN3Module.hh"
#include "TTCN3ModuleInventory.hh"
#include "ComplexType.hh"
#include "Annotation.hh"

/**
 * Type that contains information of a field of a TTCN-3 record or union
 *
 */
class AttributeType : public SimpleType {
  bool isAnyAttr;

  UseValue useVal;

  Mstring actualPath;

  Mstring nameSpace;

  bool used; // To know if already in the extended or restricted type

  TTCN3Module * origModule;

public:
  explicit AttributeType(ComplexType * a_complextype);
  AttributeType(const AttributeType & other);
  AttributeType & operator=(const AttributeType & rhs);
  void setTypeOfField(const Mstring& in);
  void setNameOfField(const Mstring& in);

  void setOrigModule(TTCN3Module * m) {
    origModule = m;
  }

  const TTCN3Module * getOrigModule() const {
    return origModule;
  }
  void setToAnyAttribute();
  void modifyValues();

  void setUseVal(UseValue use_value) {
    useVal = use_value;
  }
  void setFieldPath(const Mstring path);
  void collectVariants(List<Mstring>& container);

  UseValue getUseVal() const {
    return useVal;
  }

  bool getUsed() const {
    return used;
  }

  void setUsed(bool use) {
    used = use;
  }

  const Mstring& getNameSpaceAttribute() const {
    return nameSpace;
  }

  void addNameSpaceAttribute(Mstring namespace_) {
    if(nameSpace.empty()){
      nameSpace = namespace_;
    }else {
      nameSpace += " " + namespace_;
    }
  }
  void nameConversion_names(QualifiedNames& used);
  void applyUseAttribute();
  void applyNamespaceAttribute(VariantMode varLabel);

  void applyMinMaxOccursAttribute(unsigned long long min, unsigned long long max);

  const Mstring& getPath() const {
    return actualPath;
  }

  bool isAnyAttribute() const {
    return isAnyAttr;
  }

  void printToFile(FILE* file) {
    printToFile(file, 0);
  }
  void printToFile(FILE* file, unsigned level);

  void dump(unsigned int depth) const;

};

inline bool compareAttributeNameSpaces(AttributeType * lhs, AttributeType * rhs) {
  if (lhs->isAnyAttribute()) {
    return false;
  }
  if (lhs->getOrigModule()->getTargetNamespace() == Mstring("NoTargetNamespace") && rhs->getOrigModule()->getTargetNamespace() == Mstring("NoTargetNamespace")) {
    return false;
  } else if (lhs->getOrigModule()->getTargetNamespace() == Mstring("NoTargetNamespace")) {
    return true;
  } else if (rhs->getOrigModule()->getTargetNamespace() == Mstring("NoTargetNamespace")) {
    return false;
  } else {
    return lhs->getOrigModule()->getTargetNamespace() <= rhs->getOrigModule()->getTargetNamespace();
  }
}

inline bool compareAttributeTypes(AttributeType * lhs, AttributeType * rhs) {
  if (lhs->isAnyAttribute()) {
    return false;
  }
  if (lhs->getOrigModule()->getTargetNamespace() == rhs->getOrigModule()->getTargetNamespace()) {
    return lhs->getName().originalValueWoPrefix < rhs->getName().originalValueWoPrefix;
  } else {
    return false;
  }
}


#endif /* ATTRIBUTETYPE_HH_ */
