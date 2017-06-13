/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   >
 *   Balasko, Jeno
 *   Beres, Szabolcs
 *   Godar, Marton
 *   Raduly, Csaba
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
#ifndef SIMPLETYPE_H_
#define SIMPLETYPE_H_

#include "RootType.hh"

class SimpleType;
class Constant;

class LengthType {
  LengthType & operator=(const LengthType &); // not implemented
  // it's a bit strange that it has copy constructor but no assignment
public:
  SimpleType * parent; // no responsibility for this member
  bool modified;
  unsigned long long int facet_minLength;
  unsigned long long int facet_maxLength;
  unsigned long long int lower;
  unsigned long long int upper;

  LengthType(SimpleType * p_parent);
  // Default copy constructor and destructor are used
  
  void applyReference(const LengthType & other);
  void applyFacets();
  void printToFile(FILE * file) const;
};

class PatternType {
  PatternType & operator=(const PatternType &); // not implemented
  // it's a bit strange that it has copy constructor but no assignment
public:
  SimpleType * parent; // no responsibility for this member
  bool modified;
  Mstring facet;
  Mstring value;

  PatternType(SimpleType * p_parent);
  // Default copy constructor and destructor are used
  
  void applyReference(const PatternType & other);
  void applyFacet();
  void printToFile(FILE * file) const;
};

class EnumerationType {
  EnumerationType & operator=(const EnumerationType &); // not implemented
  // it's a bit strange that it has copy constructor but no assignment
public:
  SimpleType * parent; // no responsibility for this member
  bool modified;
  List<Mstring> facets;
  List<QualifiedName> items_string;
  List<int> items_int;
  List<double> items_float;
  List<QualifiedName> items_time;
  List<Mstring> items_misc;
  List<Mstring> variants;
  List<Mstring> converted_facets;

  EnumerationType(SimpleType * p_parent);
  // Default copy constructor and destructor are used
  
  void applyReference(const EnumerationType & other);
  void applyFacets();
  void sortFacets();
  void printToFile(FILE * file, unsigned int indent_level = 0) const;
  void insertVariants();
};

class WhitespaceType {
  WhitespaceType & operator=(const WhitespaceType &); // not implemented
  // it's a bit strange that it has copy constructor but no assignment
public:
  SimpleType * p_parent; // no responsibility for this member
  bool modified;
  Mstring facet;
  Mstring value;

  WhitespaceType(SimpleType * p_parent);
  // Default copy constructor and destructor are used

  void applyReference(const WhitespaceType & other);
  void applyFacet();
  void printToFile(FILE * file) const;
};

class ValueType {
  ValueType & operator=(const ValueType &); // not implemented
  // it's a bit strange that it has copy constructor but no assignment
public:
  SimpleType * parent; // no responsibility for this member
  bool modified;
  long double facet_minInclusive;
  long double facet_maxInclusive;
  long double facet_minExclusive;
  long double facet_maxExclusive;
  int facet_totalDigits;
  long double lower;
  long double upper;
  bool lowerExclusive;
  bool upperExclusive;
  bool not_a_number;
  Mstring fixed_value;
  Mstring default_value;
  List<Mstring> items_with_value;

  ValueType(SimpleType * p_parent);
  // Default copy constructor and destructor are used

  void applyReference(const ValueType & other);
  void applyFacets();
  void printToFile(FILE * file) const;
};

class ComplexType;

class ReferenceData {
public: // interface

  ReferenceData()
  : nst(0)
  , uri()
  , value()
  , resolved(false)
  , ref(NULL) {
  }

  void load(const Mstring& u, const Mstring& v, NamespaceType *n) {
    uri = u;
    value = v;
    nst = n;
  }

  const Mstring& get_uri() const {
    return uri;
  }

  const Mstring& get_val() const {
    return value;
  }

  RootType *get_ref() const {
    return ref;
  }

  bool empty() const {
    return uri.empty() && value.empty();
  }

  bool is_resolved() const {
    return resolved;
  }

  void set_resolved(RootType *st /*= NULL*/) {
    resolved = true;
    ref = st;
  }

  Mstring repr() const {
    return uri + Mstring("|") + value;
  }
private: // implementation
  NamespaceType *nst;
  Mstring uri;
  Mstring value;
  bool resolved;
  RootType *ref; // not owned
};

/**
 * Type that contains information coming from XSD simpleTypes, elements and attributes
 *
 * Source in XSD:
 *
 * 	* <simpleType>, <element> and <attribute> element whose parent element is <schema>
 *
 * Result in TTCN-3:
 *
 * 	* TTCN-3 type
 *
 */
class SimpleType : public RootType {
public:

  enum Mode {
    noMode,
    restrictionMode,
    extensionMode,
    listMode,
    restrictionAfterListMode
  };

protected:
  Mstring builtInBase;

  LengthType length;
  PatternType pattern;
  EnumerationType enumeration;
  WhitespaceType whitespace;
  ValueType value;

  FormValue element_form_as;
  FormValue attribute_form_as;

  Mode mode;

  ReferenceData outside_reference;

  /// true if name_dependency would be set (not empty)
  bool in_name_only;

  // True if element or attribute used with ref attribute
  bool fromRef;
  // XSD Type of the type
  TagName xsdtype;
  bool isOptional;
  Mstring substitutionGroup;
  //Pointer to the generated element substitution group
  ComplexType * subsGroup; // not owned
  //Pointer to the generated type substitution group
  ComplexType * typeSubsGroup; // not owned
  //To determine if already added to type substitution
  bool addedToTypeSubstitution;
  BlockValue block;
  
  //Special use: true if an element of a complexType is restricted to a list
  // in a simpleType, or a simpleType is a restriction of a list, which is a 
  // restriction of a simpleType
  bool inList;
  // We are inside a list if inList is true and mode == listMode
  
  // If this type is an alias then the alias field points to the original type
  SimpleType * alias; // not owned
  
  Constant * defaultForEmptyConstant; // not owned
  
  //Element substitution
  void addToSubstitutions();

  //Type substitution
  void addToTypeSubstitutions();
  //Returns the type substitution which the builtInType belongs
  ComplexType * findBuiltInTypeInStoredTypeSubstitutions(const Mstring& builtInType);

  //Only used when type substitution is enabled
  //If an element reference is found then it is put to a container
  //to know if type substitution is possible
  void collectElementTypes(SimpleType * found_ST = NULL, ComplexType * found_CT = NULL);
  void nameConversion_names();
  virtual void nameConversion_types(const List<NamespaceType> & ns);

  SimpleType & operator=(const SimpleType &); // not implemented
  // it's a bit strange that it has copy constructor but no assignment
public:
  SimpleType(XMLParser * a_parser, TTCN3Module * a_module, ConstructType a_construct);
  SimpleType(const SimpleType& other);
  // Default destructor is used

  // Parent of the element (both complexType, and AttributeType) has this
  // Not responsible for this member
  ComplexType * parent;

  /** Virtual methods
   *  inherited from RootType
   */
  void loadWithValues();
  void printToFile(FILE * file);
  void referenceResolving();
  void nameConversion(const NameConversionMode mode, const List<NamespaceType> & ns);
  void finalModification();
  virtual void finalModification2();
  virtual bool hasUnresolvedReference();
  virtual void modifyList();
  void dump(const unsigned int depth) const;

  void applyDefaultAttribute(const Mstring& default_value);
  void applyFixedAttribute(const Mstring& fixed_value);
  void applyNillableAttribute(const bool nillable_value);
  void applyAbstractAttribute(const bool abstract_value);
  void applySubstitionGroupAttribute(const Mstring& substition_group);
  void applyBlockAttribute(const BlockValue block_);
  void applyRefAttribute(const Mstring& ref_value);

  const Mstring & getBuiltInBase() const {
    return builtInBase;
  }

  const LengthType & getLength() const {
    return length;
  }

  const ValueType & getValue() const {
    return value;
  }

  const PatternType & getPattern() const {
    return pattern;
  }

  const WhitespaceType & getWhitespace() const {
    return whitespace;
  }

  const EnumerationType & getEnumeration() const {
    return enumeration;
  }

  FormValue getElementFormAs() const {
    return element_form_as;
  }

  FormValue getAttributeFormAs() const {
    return attribute_form_as;
  }

  Mode getMode() const {
    return mode;
  }

  bool isFromRef() const {
    return fromRef;
  }

  const ReferenceData& getReference() const {
    return outside_reference;
  }
  
  ReferenceData& getReference() {
    return outside_reference;
  }
    
  EnumerationType & getEnumeration() {
    return enumeration;
  }

  void setBuiltInBase(const Mstring& base) {
    builtInBase = base;
  }

  void setMode(Mode m) {
    mode = m;
  }

  void setElementFormAs(FormValue f) {
    element_form_as = f;
  }

  void setAttributeFormAs(FormValue f) {
    attribute_form_as = f;
  }

  void setReference(const Mstring& ref, bool only_name_dependency = false);

  void referenceForST(SimpleType * found_ST);
  void referenceForCT(ComplexType * found_CT);

  void setXsdtype(TagName xsdtype_) {
    xsdtype = xsdtype_;
  }

  TagName getXsdtype() const {
    return xsdtype;
  }

  ComplexType * getSubstitution() const {
      return subsGroup;
  }

  ComplexType * getTypeSubstitution() const {
      return typeSubsGroup;
  }

  BlockValue getBlock() const {
      return block;
  }
  
  void setList(const bool list) {
      inList = list;
  }
  
  void addToNameDepList(RootType * t) {
      //If the type has a substitution, we add the namedep to the substitution
      if(subsGroup != NULL && this != (SimpleType*)subsGroup){
          SimpleType * substitution = (SimpleType*)subsGroup;
          substitution->addToNameDepList(t);
      }else if(typeSubsGroup != NULL && this != (SimpleType*)typeSubsGroup){
          SimpleType * substitution = (SimpleType*)typeSubsGroup;
          substitution->addToNameDepList(t);
      }else {
          nameDepList.push_back(t);
      }
  }
  
  SimpleType * getAlias() const {
      return alias;
  }
  
  Constant * getConstantDefaultForEmpty() const {
    return defaultForEmptyConstant;
  }
  
  // Returns true if the type really restricts or extends the type not
  // just aliases it.
  bool hasRestrictionOrExtension() const;

};

#endif /* SIMPLETYPE_H_ */

