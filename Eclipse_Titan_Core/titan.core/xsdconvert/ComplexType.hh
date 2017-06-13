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
#ifndef COMPLEXTYPE_H_
#define COMPLEXTYPE_H_

#include "RootType.hh"
#include "SimpleType.hh"
#include "TTCN3Module.hh"
#include "AttributeType.hh"


class AttributeType;

/**
 * Type that contains information coming from XSD complexTypes, model group definitions
 * and attributeGroups
 *
 * Source in XSD:
 *
 * 	* <complexType>, <group> and <attributeGroup> element whose parent element is <schema>
 *    * <element> if nillable, or is a child of <complexType>
 *
 * Result in TTCN-3:
 *
 * 	* TTCN-3 type
 *
 */
class ComplexType : public SimpleType {
public:

  enum ComplexType_Mode {
    CT_simpletype_mode,
    CT_complextype_mode,
    CT_undefined_mode
  };

  enum CT_fromST {
    fromTagUnion,
    fromTagNillable,
    fromTagComplexType,
    fromTagSubstitution,
    fromTypeSubstitution
  };

  enum Resolv_State {
    No,
    InProgress,
    Yes
  };

private:
  //If the complextype is a top level component (child of schema)
  bool top;
  bool nillable;
  bool enumerated;
  bool embed;
  bool with_union;
  bool first_child;
  bool fromAll;
  bool listPrint; // true if embeddedList was true or wasInEmbeddedList was true
                  // both will be changed to false during the conversion. This
                  // variable remembers.
  unsigned long long int listMinOccurs; // MinOccurs of the list when in an element
  unsigned long long int listMaxOccurs; // MaxOccurs of the list when in an element
  unsigned int max_alt;
  int skipback;
  TagName lastType;
  Mstring actualPath;
  RootType * actfield;
  SimpleType * nameDep; // not owned
  RootType * nillable_field;
  ComplexType * basefield;
  ComplexType_Mode cmode;
  Resolv_State resolved;
  ComplexType * parentTypeSubsGroup; // not owned


  // Returns true if the attributes really restricted and not just aliased
  bool applyAttributeRestriction(ComplexType * found_CT);
  void applyAttributeExtension(ComplexType * found_CT, AttributeType * anyAttr = NULL);
  void nameConversion_names(const List<NamespaceType> & ns);
  void nameConversion_types(const List<NamespaceType> & ns);
  void nameConversion_fields(const List<NamespaceType> & ns);
  void setFieldPaths(Mstring path);
  void collectVariants(List<Mstring>& container);
  void addNameSpaceAsVariant(RootType * type, RootType * other);
  void setMinMaxOccurs(const unsigned long long min, const unsigned long long max, const bool generate_list_postfix = true);
  void applyNamespaceAttribute(VariantMode varLabel, const Mstring& ns_list);
  void applyReference(const SimpleType & other, const bool on_attributes = false);
  void setParent(ComplexType * par, SimpleType * child);
  void subFinalModification();
  void finalModification2();
  void subFinalModification2();
  Mstring findRoot(const BlockValue value, SimpleType * elem, const Mstring& head_type, const bool first);

  //Reference resolving functions
  void reference_resolving_funtion();
  void resolveAttribute(AttributeType *attr);
  void resolveAttributeGroup(SimpleType *st);
  void resolveGroup(SimpleType *st);
  void resolveElement(SimpleType *st);
  void resolveSimpleTypeExtension();
  void resolveSimpleTypeRestriction();
  void resolveComplexTypeExtension();
  void resolveComplexTypeRestriction();
  void resolveUnion(SimpleType *st);
  bool hasMatchingFields(const List<ComplexType*>& a, const List<ComplexType*>& b) const;
  // True if a restriction really restricts the type not just aliases
  bool hasComplexRestriction(ComplexType* ct) const;

  void printVariant(FILE * file);

public:
  List<ComplexType*> complexfields;
  List<AttributeType*> attribfields;
  List<Mstring> enumfields;
  List<TagName> tagNames;

  ComplexType(XMLParser * a_parser, TTCN3Module * a_module, ConstructType a_construct);
  ComplexType(ComplexType & other);
  ComplexType(const SimpleType & other, CT_fromST c);
  ComplexType(ComplexType * other);
  ~ComplexType();
  
  void modifyAttributeParent();
  void addSubstitution(SimpleType * st);
  void addTypeSubstitution(SimpleType * st);

  /** Virtual methods
   *  inherited from RootType
   */
  void loadWithValues();
  void addComment(const Mstring& text);
  void printToFile(FILE * file);
  void printToFile(FILE * file, const unsigned level, const bool is_union);

  void modifyValues();
  void referenceResolving();
  void nameConversion(NameConversionMode mode, const List<NamespaceType> & ns);
  void finalModification();
  bool hasUnresolvedReference(){ return resolved == No; }
  void modifyList();
  void setNameDep(SimpleType * dep) { nameDep = dep; }
  void setParentTypeSubsGroup(ComplexType * dep) { parentTypeSubsGroup = dep; }

  void dump(unsigned int depth) const;

};

inline bool compareComplexTypeNameSpaces(ComplexType * lhs, ComplexType * rhs) {
  if (lhs->getModule()->getTargetNamespace() == Mstring("NoTargetNamespace") && rhs->getModule()->getTargetNamespace() == Mstring("NoTargetNamespace")) {
    return false;
  } else if (lhs->getModule()->getTargetNamespace() == Mstring("NoTargetNamespace")) {
    return true;
  } else if (rhs->getModule()->getTargetNamespace() == Mstring("NoTargetNamespace")) {
    return false;
  } else {
    return lhs->getModule()->getTargetNamespace() <= rhs->getModule()->getTargetNamespace();
  }
}

inline bool compareTypes(ComplexType * lhs, ComplexType * rhs) {
  return lhs->getName().convertedValue < rhs->getName().convertedValue;
}


#endif /* COMPLEXTYPE_H_ */

