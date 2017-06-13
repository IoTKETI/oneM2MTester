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
#include "SimpleType.hh"

#include "GeneralFunctions.hh"

#include "TTCN3ModuleInventory.hh"
#include "TTCN3Module.hh"
#include "ComplexType.hh"
#include "Constant.hh"
#include "converter.hh"

#include <math.h>
#include <cfloat>

extern bool g_flag_used;
extern bool h_flag_used;

#ifndef INFINITY
#define INFINITY (DBL_MAX*DBL_MAX)
#endif
const double PLUS_INFINITY(INFINITY);
const double MINUS_INFINITY(-INFINITY);

#ifdef NAN
const double NOT_A_NUMBER(NAN);
#else
const double NOT_A_NUMBER((double)PLUS_INFINITY+(double)MINUS_INFINITY);
#endif

SimpleType::SimpleType(XMLParser * a_parser, TTCN3Module * a_module, ConstructType a_construct)
: RootType(a_parser, a_module, a_construct)
, builtInBase()
, length(this)
, pattern(this)
, enumeration(this)
, whitespace(this)
, value(this)
, element_form_as(notset)
, attribute_form_as(notset)
, mode(noMode)
, outside_reference()
, in_name_only(false)
, fromRef(false)
, xsdtype(n_NOTSET)
, isOptional(false)
, substitutionGroup(empty_string)
, subsGroup(NULL)
, typeSubsGroup(NULL)
, addedToTypeSubstitution(false)
, block(not_set)
, inList(false)
, alias(NULL)
, defaultForEmptyConstant(NULL)
, parent(NULL) {
}

SimpleType::SimpleType(const SimpleType& other)
: RootType(other)
, builtInBase(other.builtInBase)
, length(other.length)
, pattern(other.pattern)
, enumeration(other.enumeration)
, whitespace(other.whitespace)
, value(other.value)
, element_form_as(other.element_form_as)
, attribute_form_as(other.attribute_form_as)
, mode(other.mode)
, outside_reference(other.outside_reference)
, in_name_only(other.in_name_only)
, fromRef(other.fromRef)
, xsdtype(other.xsdtype)
, isOptional(other.isOptional)
, substitutionGroup(other.substitutionGroup)
, subsGroup(other.subsGroup)
, typeSubsGroup(other.typeSubsGroup)
, addedToTypeSubstitution(other.addedToTypeSubstitution)
, block(other.block)
, inList(other.inList)
, alias(other.alias)
, defaultForEmptyConstant(other.defaultForEmptyConstant)
, parent(NULL) {
  length.parent = this;
  pattern.parent = this;
  enumeration.parent = this;
  whitespace.p_parent = this;
  value.parent = this;
}

void SimpleType::loadWithValues() {
  const XMLParser::TagAttributes & atts = parser->getActualTagAttributes();
  switch (parser->getActualTagName()) {
    case n_restriction:
      type.upload(atts.base);
      setReference(atts.base);
      mode = restrictionMode;
      break;
    case n_list:
      type.upload(atts.itemType);
      setReference(atts.itemType);
      setMinOccurs(0);
      setMaxOccurs(ULLONG_MAX);
      addVariant(V_list);
      mode = listMode;
      inList = true;
      break;
    case n_union:
    { // generating complextype from simpletype
      ComplexType * new_complextype = new ComplexType(*this, ComplexType::fromTagUnion);
      new_complextype->loadWithValues();
      break;
    }
    case n_element:
      name.upload(atts.name);
      type.upload(atts.type);
      setReference(atts.type, true);
      if (!atts.nillable) {
        applyDefaultAttribute(atts.default_);
        applyFixedAttribute(atts.fixed);
      }
      applyAbstractAttribute(atts.abstract);
      applySubstitionGroupAttribute(atts.substitionGroup);
      applyBlockAttribute(atts.block);
      //This shall be the last instruction always
      applyNillableAttribute(atts.nillable);
      break;
    case n_attribute:
      name.upload(atts.name);
      type.upload(atts.type);
      xsdtype = n_attribute;
      setReference(atts.type, true);
      applyDefaultAttribute(atts.default_);
      applyFixedAttribute(atts.fixed);
      break;
    case n_simpleType:
      name.upload(atts.name);
      break;
    case n_complexType:
    { // generating complextype from simpletype
      ComplexType * new_complextype = new ComplexType(*this, ComplexType::fromTagComplexType);
      new_complextype->loadWithValues();
      break;
    }
    case n_length:
       if (inList && (xsdtype != n_NOTSET || mode == restrictionAfterListMode)) {
        setMinOccurs(strtoull(atts.value.c_str(), NULL, 0));
        setMaxOccurs(strtoull(atts.value.c_str(), NULL, 0));
        break;
      }
      length.facet_minLength = strtoull(atts.value.c_str(), NULL, 0);
      length.facet_maxLength = strtoull(atts.value.c_str(), NULL, 0);
      length.modified = true;
      break;
    case n_minLength:
      if (inList && (xsdtype != n_NOTSET || mode == restrictionAfterListMode)) {
        setMinOccurs(strtoull(atts.value.c_str(), NULL, 0));
        break;
      }
      length.facet_minLength = strtoull(atts.value.c_str(), NULL, 0);
      length.modified = true;
      break;
    case n_maxLength:
      if (inList && (xsdtype != n_NOTSET || mode == restrictionAfterListMode)) {
        setMaxOccurs(strtoull(atts.value.c_str(), NULL, 0));
        break;
      }
      length.facet_maxLength = strtoull(atts.value.c_str(), NULL, 0);
      length.modified = true;
      break;
    case n_pattern:
      pattern.facet = atts.value;
      pattern.modified = true;
      break;
    case n_enumeration:
      enumeration.facets.push_back(atts.value);
      enumeration.modified = true;
      break;
    case n_whiteSpace:
      whitespace.facet = atts.value;
      whitespace.modified = true;
      break;
    case n_minInclusive:
      if (atts.value == "NaN") {
        value.not_a_number = true;
      } else if (atts.value == "-INF") {
        value.facet_minInclusive = -DBL_MAX;
      } else if (atts.value == "INF") {
        value.facet_minInclusive = DBL_MAX;
      } else {
        value.facet_minInclusive = stringToLongDouble(atts.value.c_str());
      }
      value.modified = true;
      break;
    case n_maxInclusive:
      if (atts.value == "NaN") {
        value.not_a_number = true;
      } else if (atts.value == "-INF") {
        value.facet_maxInclusive = -DBL_MAX;
      } else if (atts.value == "INF") {
        value.facet_maxInclusive = DBL_MAX;
      } else {
        value.facet_maxInclusive = stringToLongDouble(atts.value.c_str());
      }
      value.modified = true;
      break;
    case n_minExclusive:
      if (atts.value == "NaN") {
        setInvisible();
      } else if (atts.value == "-INF") {
        value.facet_minExclusive = -DBL_MAX;
      } else if (atts.value == "INF") {
        setInvisible();
      } else {
        value.facet_minExclusive = stringToLongDouble(atts.value.c_str());
      }
      value.modified = true;
      value.lowerExclusive = true;
      break;
    case n_maxExclusive:
      if (atts.value == "NaN") {
        setInvisible();
      } else if (atts.value == "-INF") {
        setInvisible();
      } else if (atts.value == "INF") {
        value.facet_maxExclusive = DBL_MAX;
      } else {
        value.facet_maxExclusive = stringToLongDouble(atts.value.c_str());
      }
      value.modified = true;
      value.upperExclusive = true;
      break;
    case n_totalDigits:
      value.facet_totalDigits = strtoul(atts.value.c_str(), NULL, 0);
      value.modified = true;
      break;
    case n_fractionDigits:
      addVariant(V_fractionDigits, atts.value);
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

void SimpleType::applyDefaultAttribute(const Mstring& default_value) {
  if (!default_value.empty()) {
    value.default_value = default_value;
    const Mstring typeT = type.originalValueWoPrefix.getValueWithoutPrefix(':');
    //Not supported for hexBinary and base64Binary
    if (typeT != "hexBinary" && typeT != "base64Binary") {
      Constant * c = new Constant(this, type.convertedValue, default_value);
      module->addConstant(c);
      defaultForEmptyConstant = c;
      // Delay adding the defaultForEmpty variant after the nameconversion
      // happened on the constants
    }
  }
}

void SimpleType::applyFixedAttribute(const Mstring& fixed_value) {
  if (!fixed_value.empty()) {
    value.fixed_value = fixed_value;
    value.modified = true;
    const Mstring typeT = type.originalValueWoPrefix.getValueWithoutPrefix(':');
    //Not supported for hexBinary and base64Binary
    if (typeT != "hexBinary" && typeT != "base64Binary") {
      Constant * c = new Constant(this, type.convertedValue, fixed_value);
      module->addConstant(c);
      defaultForEmptyConstant = c;
      // Delay adding the defaultForEmpty variant after the nameconversion
      // happened on the constants
    }
  }
}

void SimpleType::applyNillableAttribute(const bool nillable) {
  if (nillable) {
    ComplexType * new_complextype = new ComplexType(*this, ComplexType::fromTagNillable); // generating complextype from simpletype
    new_complextype->loadWithValues();
  }
}

void SimpleType::applyAbstractAttribute(const bool abstract_value) {
  if (abstract_value) {
    addVariant(V_abstract);
  }
}

void SimpleType::applySubstitionGroupAttribute(const Mstring& substitution_group){
  if(!substitution_group.empty()){
    substitutionGroup = substitution_group;
  }
}

void SimpleType::applyBlockAttribute(const BlockValue block_){
  if(block_ == not_set){
    block = getModule()->getBlockDefault();
  }else {
    block = block_;
  }
}

void SimpleType::addToSubstitutions(){
  if(!g_flag_used || substitutionGroup.empty()){
    return;
  }
  SimpleType * st_ = (SimpleType*) TTCN3ModuleInventory::getInstance().lookup(this, substitutionGroup, want_BOTH);
  if(st_ == NULL){
    printError(module->getSchemaname(), name.convertedValue,
        "Reference for a non-defined type: " + substitutionGroup);
      TTCN3ModuleInventory::getInstance().incrNumErrors();
    return;
  }
  SimpleType * st = (SimpleType*)st_;
  if(st->getSubstitution() != NULL){
    st = st->getSubstitution();
  }

  st->referenceResolving();
  substitutionGroup = empty_string;
  //Simpletype
  if(st->subsGroup == NULL){
    ComplexType * head_element = new ComplexType(*st, ComplexType::fromTagSubstitution);
    for(List<RootType*>::iterator simpletype = st->nameDepList.begin(); simpletype; simpletype = simpletype->Next){
      head_element->getNameDepList().push_back(simpletype->Data);
    }
    st->nameDepList.clear();
    st->getModule()->addMainType(head_element);
    head_element->addVariant(V_untagged);
    head_element->addSubstitution(st);
    head_element->addSubstitution(this);
    //if st->subsGroup == this, then it is a generated subs group
    st->subsGroup = head_element;
    st->setInvisible();
  }else {
    st->subsGroup->addSubstitution(this);
  }
}

void SimpleType::addToTypeSubstitutions() {
  //If the user did not request type substitution generation or
  //the type is already added to type substitution
  if(!h_flag_used || addedToTypeSubstitution){
    return;
  }
  //Only available if it is a restricion or extension
  if(mode != extensionMode && mode != restrictionMode){
    return;
  }
  //Only top level complexTypes or simpleTypes, ergo no elements
  if(parent != NULL || hasVariant(Mstring("\"element\""))){
    return;
  }

  //It would be nice if here outside_reference.resolved to everything
  bool newST = false;
  SimpleType * st = (SimpleType*)outside_reference.get_ref();
  bool isConvertedBuiltIn = isBuiltInType(type.convertedValue);
  if(st == NULL && !isConvertedBuiltIn){
    //Not even a reference, and not a built in type
    return;
  }else if(st == NULL && isConvertedBuiltIn){
    st = new SimpleType(parser, module, construct);
    st->type.upload(type.convertedValue);
    st->name.upload(type.convertedValue);
    st->typeSubsGroup = findBuiltInTypeInStoredTypeSubstitutions(type.convertedValue);
    outside_reference.set_resolved(st);
    //Add this decoy type to the maintypes -> module will free st
    //st->setInvisible();
    module->addMainType(st);
    newST = true;
  }

  addedToTypeSubstitution = true;
  st->addToTypeSubstitutions();
  //If type substitution is NULL then we need to create the union
  if(st->getTypeSubstitution() == NULL){
    ComplexType * head_element = new ComplexType(*st, ComplexType::fromTypeSubstitution);
    head_element->getNameDepList().clear();
    for(List<RootType*>::iterator roottype = st->nameDepList.begin(), nextST; roottype; roottype = nextST){
      nextST = roottype->Next;
      //Don't add if it is in a type substitution 
      SimpleType* simpletype = dynamic_cast<SimpleType*>(roottype->Data);
      if (simpletype == NULL) continue;
      if(simpletype->getTypeSubstitution() == NULL){
        head_element->getNameDepList().push_back(roottype->Data);
        st->getNameDepList().remove(roottype);
      }
    }
    
    st->getModule()->addMainType(head_element);
    head_element->addVariant(V_useType);
    //For cascading type substitution
    if(st->outside_reference.get_ref() != NULL && ((ComplexType*)st->outside_reference.get_ref())->getTypeSubstitution() != NULL){
      head_element->setParentTypeSubsGroup(((ComplexType*)st->outside_reference.get_ref())->getTypeSubstitution());
    }
    head_element->addTypeSubstitution(st);
    head_element->addTypeSubstitution(this);
    bool found = false;
    //Check to find if there was already an element reference with this type
    for(List<typeNameDepList>::iterator str = module->getElementTypes().begin(); str; str = str->Next){
      Mstring prefix = str->Data.type.getPrefix(':');
      Mstring value_ = str->Data.type.getValueWithoutPrefix(':');

      if((value_ == st->getName().convertedValue.getValueWithoutPrefix(':') && prefix == module->getTargetNamespaceConnector()) ||
         (isBuiltInType(value_) && !isBuiltInType(st->getType().convertedValue) && value_ == st->getType().convertedValue && prefix == module->getTargetNamespaceConnector())){
        //Push the namedeplist
        for(List<SimpleType*>::iterator simpletype = str->Data.nameDepList.begin(); simpletype; simpletype = simpletype->Next){
          head_element->getNameDepList().push_back(simpletype->Data);
        }
        found = true;
        str->Data.typeSubsGroup = head_element;
        break;
      }
    }
    if(!found){
      head_element->setInvisible();
    }
    st->typeSubsGroup = head_element;
    st->getModule()->addStoredTypeSubstitution(head_element);
  }else {
    st->getTypeSubstitution()->addTypeSubstitution(this);
  }
  if(newST){
    //Make the decoy invisible
    st->setInvisible();
  }
}

void SimpleType::collectElementTypes(SimpleType* found_ST, ComplexType* found_CT){
  //Only if type substitution is enabled and it is a top level(simpletype) element or
  //it is a not top level element(complextype)
  if(h_flag_used && (hasVariant(Mstring("\"element\"")) || xsdtype == n_element)){
    SimpleType * st =  NULL, *nameDep = NULL;
    Mstring uri, value_, type_;
    if(found_ST != NULL || found_CT != NULL){
      // st := found_ST or found_CT, which is not null
      st = found_ST != NULL ? found_ST : found_CT;
      uri = outside_reference.get_uri();
      value_ = outside_reference.get_val();
      type_ = value_;
    }else if(isBuiltInType(type.convertedValue)){
      st = this;
      uri = module->getTargetNamespace();
      value_ = type.convertedValue;
      if(outside_reference.empty()){
        type_ = value_;
        nameDep = this;
      }else {
        type_ = outside_reference.get_val();
      }
    }else {
      //It is not possible to reach here (should be)
      return;
    }
    type_ = type_.getValueWithoutPrefix(':');
    bool found = false;
    const Mstring typeSubsName = value_ + Mstring("_derivations");
    //Find if we already have a substitution type to this element reference
    for(List<ComplexType*>::iterator complex = st->getModule()->getStoredTypeSubstitutions().begin(); complex; complex = complex->Next){

      if(uri == st->getModule()->getTargetNamespace() && typeSubsName == complex->Data->getName().convertedValue){
        complex->Data->setVisible();
        if(st->getXsdtype() != n_NOTSET && this == st){ //otherwise records would be renamed too
          complex->Data->addToNameDepList(st);
          ((ComplexType*)st)->setNameDep(nameDep);
        }
        found = true;
        break;
      }
    }
    //Add the reference, to future possible type substitution
    if(!found){
      Mstring prefix = st->getModule()->getTargetNamespaceConnector();
      if(prefix != empty_string){
        prefix += ":";
      }
      st->getModule()->addElementType(prefix + type_, nameDep);
    }
  }
}

ComplexType * SimpleType::findBuiltInTypeInStoredTypeSubstitutions(const Mstring& builtInType){
  const Mstring typeSubsName = builtInType.getValueWithoutPrefix(':') + Mstring("_derivations");
  for(List<ComplexType*>::iterator complex = module->getStoredTypeSubstitutions().begin(); complex; complex = complex->Next){
    if(typeSubsName == complex->Data->getName().convertedValue){
      return complex->Data;
    }
  }
  return NULL;
}


void SimpleType::setReference(const Mstring& ref, bool only_name_dependency) {
  if (ref.empty()) {
    return;
  }
  if (isBuiltInType(ref)) {
    builtInBase = ref.getValueWithoutPrefix(':');
    if (name.convertedValue.empty()) {
      name.upload(ref);
    }
    if (type.convertedValue.empty() || type.convertedValue == "anySimpleType") {
      type.upload(ref);
    }
    bool found = false;
    for (List<NamespaceType>::iterator ns = getModule()->getDeclaredNamespaces().begin(); ns; ns = ns->Next) {
      if (ns->Data.uri != XMLSchema && type.refPrefix == ns->Data.prefix.c_str()) {
        found = true;
        break;
      }
    }
    fromRef = true;
    if (!found) {
      return;
    }
  }

  Mstring refPrefix = ref.getPrefix(':');
  Mstring refValue = ref.getValueWithoutPrefix(':');
  Mstring refUri;
  // Find the URI amongst the known namespace URIs
  List<NamespaceType>::iterator declNS;
  for (declNS = module->getDeclaredNamespaces().begin(); declNS; declNS = declNS->Next) {
    if (refPrefix == declNS->Data.prefix) {
      refUri = declNS->Data.uri;
      break;
    }
  }

  // FIXME: can this part be moved above the search ?
  if (refUri.empty()) { // not found
    if (refPrefix == "xml") {
      refUri = "http://www.w3.org/XML/1998/namespace";
    } else if (refPrefix == "xmlns") {
      refUri = "http://www.w3.org/2000/xmlns";
    } else if (refPrefix.empty() && module->getTargetNamespace() == "NoTargetNamespace") {
      refUri = "NoTargetNamespace";
    }
  }

  if (refUri.empty()) { // something is incorrect - unable to find the uri to the given prefix
    if (refPrefix.empty()) {
      printError(module->getSchemaname(), parser->getActualLineNumber(),
        Mstring("The absent namespace must be imported because "
        "it is not the same as the target namespace of the current schema."));
      parser->incrNumErrors();
      return;
    } else {
      printError(module->getSchemaname(), parser->getActualLineNumber(),
        "The value \'" + ref + "\' is incorrect: "
        "A namespace prefix does not denote any URI.");
      parser->incrNumErrors();
      return;
    }
  }

  if (only_name_dependency) {
    in_name_only = true;
  }

  outside_reference.load(refUri, refValue, &declNS->Data);
}

void SimpleType::referenceResolving() {
  if (outside_reference.empty() || outside_reference.is_resolved()){
    addToTypeSubstitutions();
    collectElementTypes();
  }
  if(outside_reference.empty() && substitutionGroup.empty()) return;
  if (outside_reference.is_resolved()) return;
  
  if(!outside_reference.empty()){
    SimpleType * found_ST = static_cast<SimpleType*> (
      TTCN3ModuleInventory::getInstance().lookup(this, want_ST));
    ComplexType * found_CT = static_cast<ComplexType*> (
      TTCN3ModuleInventory::getInstance().lookup(this, want_CT));
    // It _is_ possible to find both
    collectElementTypes(found_ST, found_CT);
    if (found_ST != NULL) {
      if (!found_ST->outside_reference.empty() && !found_ST->outside_reference.is_resolved() && found_ST != this) {
        found_ST->referenceResolving();
        if(!found_ST->outside_reference.is_resolved()){
          found_ST->outside_reference.set_resolved(NULL);
        }
      }
      referenceForST(found_ST);
      addToTypeSubstitutions();
      if (!isBuiltInType(type.convertedValue)) {
        found_ST->addToNameDepList(this);
      }
    } else if (found_CT != NULL) {
      referenceForCT(found_CT);
      addToTypeSubstitutions();
      if (!isBuiltInType(type.convertedValue)) {
        found_CT->addToNameDepList(this);
      }
    }else {
      printError(module->getSchemaname(), name.convertedValue,
        "Reference for a non-defined type: " + outside_reference.repr());
      TTCN3ModuleInventory::getInstance().incrNumErrors();
      outside_reference.set_resolved(NULL);
    }
    addToSubstitutions();
  }else {
    addToSubstitutions();
  }
}

void SimpleType::referenceForST(SimpleType * found_ST) {
  while(found_ST != NULL && found_ST->alias != NULL){
    found_ST = found_ST->alias;
  }
  outside_reference.set_resolved(found_ST);
  if (in_name_only)
    return;

  if (!found_ST->builtInBase.empty()) {
    builtInBase = found_ST->builtInBase;
  }

  if (construct == c_element)
    return;

  if (mode == listMode || mode == restrictionAfterListMode)
    return;
  
  bool hasRestrOrExt = hasRestrictionOrExtension();
  
  length.applyReference(found_ST->length);
  pattern.applyReference(found_ST->pattern);
  enumeration.applyReference(found_ST->enumeration);
  whitespace.applyReference(found_ST->whitespace);
  value.applyReference(found_ST->value);
  
  if(!hasRestrOrExt){
    length.modified = false;
    pattern.modified = false;
    enumeration.modified = false;
    whitespace.modified = false;
    value.modified = false;
    alias = found_ST;
  }

  mode = found_ST->mode;
  if (found_ST->mode != listMode && found_ST->mode != restrictionAfterListMode
      && hasRestrOrExt) {
    type.upload(found_ST->getType().convertedValue);
  }
}

void SimpleType::referenceForCT(ComplexType * found_CT) {
  outside_reference.set_resolved(found_CT);

  if (in_name_only)
    return;

  // Section 7.5.3 Example5
  if (found_CT->getType().convertedValue == Mstring("union") && mode == restrictionMode) {
    for (List<Mstring>::iterator facet = enumeration.facets.begin(); facet; facet = facet->Next) {
      enumeration.items_misc.push_back(facet->Data);
    }
  }
  size_t value_size = value.items_with_value.size(); //Used to check changes
  enumeration.modified = false;
  for (List<Mstring>::iterator itemMisc = enumeration.items_misc.begin(); itemMisc; itemMisc = itemMisc->Next) {
    size_t act_size = value.items_with_value.size(); //Used to detect if field did not match any field
    for (List<ComplexType*>::iterator field = found_CT->complexfields.begin(); field; field = field->Next) {
      if (isIntegerType(field->Data->getType().convertedValue)) {
        int read_chars = -1;
        int val = -1;
        sscanf(itemMisc->Data.c_str(), "%d%n", &val, &read_chars);
        if ((size_t) read_chars == itemMisc->Data.size()) {
          expstring_t tmp_string = mprintf("{%s:=%d}",
            field->Data->getName().convertedValue.c_str(), val);
          value.items_with_value.push_back(Mstring(tmp_string));
          Free(tmp_string);
          break;
        }
      }

      if (isFloatType(field->Data->getType().convertedValue)) {
        int read_chars = -1;
        float val = -1.0;
        sscanf(itemMisc->Data.c_str(), "%f%n", &val, &read_chars);
        if ((size_t) read_chars == itemMisc->Data.size()) {
          expstring_t tmp_string = mprintf("{%s:=%f}",
            field->Data->getName().convertedValue.c_str(), val);
          value.items_with_value.push_back(Mstring(tmp_string));
          Free(tmp_string);
          break;
        }
      }

      if (isTimeType(field->Data->getType().convertedValue)) {
        if (matchDates(itemMisc->Data.c_str(), field->Data->getType().originalValueWoPrefix.c_str())) {
          expstring_t tmp_string = mprintf("{%s:=\"%s\"}",
            field->Data->getName().convertedValue.c_str(), itemMisc->Data.c_str());
          value.items_with_value.push_back(Mstring(tmp_string));
          Free(tmp_string);
          break;
        }
      }

      if (isStringType(field->Data->getType().convertedValue)) {
        expstring_t tmp_string = mprintf("{%s:=\"%s\"}",
          field->Data->getName().convertedValue.c_str(), itemMisc->Data.c_str());
        value.items_with_value.push_back(Mstring(tmp_string));
        Free(tmp_string);
        break;
      }
    }

    if (act_size == value.items_with_value.size()) {
      printWarning(getModule()->getSchemaname(), getName().convertedValue,
        Mstring("The following enumeration did not match any field: ") + itemMisc->Data + Mstring("."));
      TTCN3ModuleInventory::getInstance().incrNumWarnings();
    }
  }

  if (value_size != value.items_with_value.size()) {
    value.modified = true;
  }
}

void SimpleType::nameConversion(NameConversionMode conversion_mode, const List<NamespaceType> & ns) {
  if(!visible) return;
  switch (conversion_mode) {
    case nameMode:
      nameConversion_names();
      break;
    case typeMode:
      nameConversion_types(ns);
      break;
    case fieldMode:
      break;
  }
}

void SimpleType::nameConversion_names() {
  Mstring res, var(module->getTargetNamespace());
  XSDName2TTCN3Name(name.convertedValue, TTCN3ModuleInventory::getInstance().getTypenames(), type_name, res, var);
  name.convertedValue = res;
  addVariant(V_onlyValue, var);
  for (List<RootType*>::iterator st = nameDepList.begin(); st; st = st->Next) {
    st->Data->setTypeValue(res);
  }
  if (outside_reference.get_ref() != NULL && defaultForEmptyConstant!= NULL) {
    // We don't know if the name conversion already happened on the get_ref()
    // so we push defaultForEmptyConstant to it's namedeplist.
    defaultForEmptyConstant->setTypeValue(outside_reference.get_ref()->getType().convertedValue);
    outside_reference.get_ref()->getNameDepList().push_back(defaultForEmptyConstant);
  }
}

void SimpleType::nameConversion_types(const List<NamespaceType> & ns) {
  if (type.convertedValue == "record" || type.convertedValue == "set"
    || type.convertedValue == "union" || type.convertedValue == "enumerated") return;

  Mstring prefix = type.convertedValue.getPrefix(':');
  Mstring value_str = type.convertedValue.getValueWithoutPrefix(':');

  Mstring uri;
  for (List<NamespaceType>::iterator namesp = ns.begin(); namesp; namesp = namesp->Next) {
    if (prefix == namesp->Data.prefix) {
      uri = namesp->Data.uri;
      break;
    }
  }

  QualifiedName tmp(uri, value_str);

  QualifiedNames::iterator origTN = TTCN3ModuleInventory::getInstance().getTypenames().begin();
  for (; origTN; origTN = origTN->Next) {
    if (tmp == origTN->Data) {
      QualifiedName tmp_name(module->getTargetNamespace(), name.convertedValue);
      if (tmp_name == origTN->Data)
        continue; // get a new type name
      else
        break;
    }
  }
  if (origTN != NULL) {
    setTypeValue(origTN->Data.name);
    // This      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ is always value_str
    // The only effect here is to remove the "xs:" prefix from type.convertedValue,
    // otherwise the new value is always the same as the old.
  } else {
    Mstring res, var;
    XSDName2TTCN3Name(value_str, TTCN3ModuleInventory::getInstance().getTypenames(), type_reference_name, res, var);
    setTypeValue(res);
  }
}

void SimpleType::finalModification() {
  value.applyFacets();
  length.applyFacets();
  pattern.applyFacet();
  whitespace.applyFacet();
  enumeration.applyFacets();

  if (module->getElementFormDefault() == qualified &&
    element_form_as == unqualified) {
    addVariant(V_formAs, Mstring("unqualified"));
  } else if (module->getElementFormDefault() != qualified &&
    element_form_as == qualified) {
    addVariant(V_formAs, Mstring("qualified"));
  }

  if (module->getAttributeFormDefault() == qualified &&
    attribute_form_as == unqualified) {
    addVariant(V_formAs, Mstring("unqualified"));
  } else if (module->getAttributeFormDefault() != qualified &&
    attribute_form_as == qualified) {
    addVariant(V_formAs, Mstring("qualified"));
  }

  if (type.originalValueWoPrefix == Mstring("boolean")) {
    addVariant(V_onlyValueHidden, Mstring("\"text 'false' as '0'\""));
    addVariant(V_onlyValueHidden, Mstring("\"text 'true' as '1'\""));
  }

  isOptional = isOptional || (getMinOccurs() == 0 && getMaxOccurs() == 0);
  

}

void SimpleType::finalModification2() {
  // Delayed adding the defaultForEmpty variant after the nameconversion
  // happened on the constants
  if (defaultForEmptyConstant != NULL) {
    TTCN3Module * mod = parent != NULL ? parent->getModule() : getModule();
    addVariant(V_defaultForEmpty, defaultForEmptyConstant->getAlterego()->getConstantName(mod));
  }
}

bool SimpleType::hasUnresolvedReference() {
  if (!outside_reference.empty() && !outside_reference.is_resolved()) {
    return true;
  } else {
    return false;
  }
}

void SimpleType::modifyList() {
  // Intentional empty
}

void SimpleType::applyRefAttribute(const Mstring& ref_value) {
  if (!ref_value.empty()) {
    setReference(ref_value);
    fromRef = true;
  }
}

bool SimpleType::hasRestrictionOrExtension() const {
  return
    enumeration.modified ||
    length.modified ||
    value.modified ||
    pattern.modified ||
    whitespace.modified;
}

void SimpleType::printToFile(FILE * file) {
  if (!visible) {
    return;
  }

  printComment(file);

  fputs("type ", file);
  if(enumeration.modified && hasVariant(Mstring("\"list\""))){
    printMinOccursMaxOccurs(file, false);
    fprintf(file, "enumerated\n{\n");
    enumeration.sortFacets();
    enumeration.printToFile(file);
    fprintf(file, "\n} %s", name.convertedValue.c_str());
  } else if (enumeration.modified) {
    if (isFloatType(builtInBase)) {
      fprintf(file, "%s %s (", type.convertedValue.c_str(), name.convertedValue.c_str());
      enumeration.sortFacets();
      enumeration.printToFile(file);
      fputc(')', file);
    } else {
      fprintf(file, "enumerated %s\n{\n", name.convertedValue.c_str());
      enumeration.sortFacets();
      enumeration.printToFile(file);
      fputs("\n}", file);
    }
  } else {
    printMinOccursMaxOccurs(file, false);

    int multiplicity = multi(module, outside_reference, this);
    const RootType *type_ref = outside_reference.get_ref();
    if ((multiplicity > 1 && type_ref && type_ref->getModule() != module)
       || name.convertedValue == type.convertedValue) {
      fprintf(file, "%s.", type_ref->getModule()->getModulename().c_str());
    }

    fprintf(file, "%s %s",
      type.convertedValue.c_str(), name.convertedValue.c_str());
    pattern.printToFile(file);
    value.printToFile(file);
    length.printToFile(file);
  }
  enumeration.insertVariants();
  printVariant(file);
  fputs(";\n\n\n", file);
}

void SimpleType::dump(unsigned int depth) const {
  static const char *modes[] = {
    "", "restriction", "extension", "list", "restrictionAfterList"
  };
  fprintf(stderr, "%*s SimpleType '%s' -> '%s' at %p\n", depth * 2, "",
    name.originalValueWoPrefix.c_str(), name.convertedValue.c_str(),
    (const void*) this);
  fprintf(stderr, "%*s   type '%s' -> '%s'\n", depth * 2, "",
    type.originalValueWoPrefix.c_str(), type.convertedValue.c_str());

  if (mode != noMode) {
    fprintf(stderr, "%*s   %s, base='%s'\n", depth * 2, "", modes[mode], builtInBase.c_str());
  }

  //  fprintf  (stderr, "%*s   rfo='%s' n_d='%s'\n", depth * 2, "",
  //    reference_for_other.c_str(), name_dependency.c_str());
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

LengthType::LengthType(SimpleType * a_simpleType)
: parent(a_simpleType)
, modified(false)
, facet_minLength(0)
, facet_maxLength(ULLONG_MAX)
, lower(0)
, upper(ULLONG_MAX) {
}

void LengthType::applyReference(const LengthType & other) { 
  if (!modified) modified = other.modified;
  if (other.facet_minLength > facet_minLength) facet_minLength = other.facet_minLength;
  if (other.facet_maxLength < facet_maxLength) facet_maxLength = other.facet_maxLength;
}

void LengthType::applyFacets() // only for string types and list types without QName
{
  if (!modified) return;

  switch (parent->getMode()) {
    case SimpleType::restrictionMode:
    {
      const Mstring & base = parent->getBuiltInBase();
      if ((isStringType(base) || (isSequenceType(base) && base != "QName") || isAnyType(base)) || base.empty()) {
        lower = facet_minLength;
        upper = facet_maxLength;
      } else {
        printWarning(parent->getModule()->getSchemaname(), parent->getName().convertedValue,
          Mstring("Length restriction is not supported on type '") + base + Mstring("'."));
        TTCN3ModuleInventory::getInstance().incrNumWarnings();
      }
      break;
    }
    case SimpleType::extensionMode:
    case SimpleType::listMode:
    case SimpleType::restrictionAfterListMode:
      lower = facet_minLength;
      upper = facet_maxLength;
      break;
    case SimpleType::noMode:
      break;
  }
  if (lower > upper) {
    printError(parent->getModule()->getSchemaname(), parent->getName().convertedValue,
      Mstring("The upper boundary of length restriction cannot be smaller than the lower boundary."));
    TTCN3ModuleInventory::getInstance().incrNumErrors();
    return;
  }
}

void LengthType::printToFile(FILE * file) const {
  if (!modified) return;
  if (parent->getEnumeration().modified) return;

  if (lower == upper) {
    fprintf(file, " length(%llu)", lower);
  } else {
    fprintf(file, " length(%llu .. ", lower);

    if (upper == ULLONG_MAX) {
      fputs("infinity", file);
    } else {
      fprintf(file, "%llu", upper);
    }

    fputc(')', file);
  }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

PatternType::PatternType(SimpleType * a_simpleType)
: parent(a_simpleType)
, modified(false)
, facet()
, value() {
}

void PatternType::applyReference(const PatternType & other) {
  if (!modified) modified = other.modified;
  if (facet.empty()) facet = other.facet;
}

void PatternType::applyFacet() // only for time types and string types without hexBinary
{
  if (!modified) return;

  const Mstring & base = parent->getBuiltInBase();
  if (((isStringType(base) && base != "hexBinary") || isTimeType(base) || isAnyType(base)) || base.empty()) {
    // XSD pattern to TTCN-3 pattern; ETSI ES 201 873-9 clause 6.1.4
    // FIXME a proper scanner is needed, e.g. from flex
    int charclass = 0;
    for (size_t i = 0; i != facet.size(); ++i) {
      char c = facet[i];
      switch (c) {
        case '(':
          value += charclass ? "\\("
            : "(";
          break;
        case ')':
          value += charclass ? "\\)"
            : ")";
          break;
        case '/':
          value += '/';
          break;
        case '^':
          value += charclass ? "\\^"
            : "^";
          break;
        case '[':
          value += c;
          ++charclass;
          break;
        case ']':
          value += c;
          --charclass;
          break;
        case '.': // any character
          value += charclass ? '.'
            : '?';
          break;
        case '*': // 0 or more
          value += '*'; //#(0,)
          break;
        case '+':
          value += '+'; //#(1,)
          break;
        case '?':
          value += charclass ? "?"
            : "#(0,1)";
          break;
        case '"':
          value += "\"\"";
          break;
        case '{':
        {
          if (charclass == 0) {
            Mstring s;
            int k = 1;
            while (facet[i + k] != '}') {
              s += facet[i + k];
              ++k;
            }
            int a, b, match;
            match = sscanf(s.c_str(), "%i,%i", &a, &b);
            if (match == 1 || match == 2) {
              value += "#(";
              value += s;
              value += ')';
              i = i + k;
            } else {
              value += "\\{";
            }
          } else {
            value += "\\{";
          }
          break;
        }
        case '}':
          value += charclass ? "\\}"
            : "}";
          break;
        case '\\':
        {
          // Appendix G1.1 of XML Schema Datatypes: Character class escapes;
          // specifically, http://www.w3.org/TR/xmlschema11-2/#nt-MultiCharEsc
          char cn = facet[i + 1];
          switch (cn) {
            case 'c':
              value += charclass ? "\\w\\d.\\-_:"
                : "[\\w\\d.\\-_:]";
              break;
            case 'C':
              value += charclass ? "^\\w\\d.\\-_:"
                : "[^\\w\\d.\\-_:]";
              break;
            case 'D':
              value += charclass ? "^\\d"
                : "[^\\d]";
              break;
            case 'i':
              value += charclass ? "\\w\\d:"
                : "[\\w\\d:]";
              break;
            case 'I':
              value += charclass ? "^\\w\\d:"
                : "[^\\w\\d:]";
              break;
            case 's':
              value += charclass ? "\\q{0,0,0,20}\\q{0,0,0,10}\\t\\r"
                : "[\\q{0,0,0,20}\\q{0,0,0,10}\\t\\r]";
              break;
            case 'S':
              value += charclass ? "^\\q{0,0,0,20}\\q{0,0,0,10}\\t\\r"
                : "[^\\q{0,0,0,20}\\q{0,0,0,10}\\t\\r]";
              break;
            case 'W':
              value += charclass ? "^\\w"
                : "[^\\w]";
              break;
            case 'p':
              printWarning(parent->getModule()->getSchemaname(), parent->getName().convertedValue,
                Mstring("Character categories and blocks are not supported."));
              TTCN3ModuleInventory::getInstance().incrNumWarnings();
              parent->addComment(
                Mstring("Pattern is not converted due to using character categories and blocks in patterns is not supported."));
              value.clear();
              return;

            case '.':
              value += '.';
              break;
            default:
              // backslash + another: pass unmodified; this also handles \d and \w
              value += c;
              value += cn;
              break;
          }
          ++i;
          break;
        }
        case '&':
          if (facet[i + 1] == '#') { // &#....;
            Mstring s;
            int k = 2;
            while (facet[i + k] != ';') {
              s += facet[i + k];
              ++k;
            }
            long long int d = atoll(s.c_str());
            if (d < 0 || d > 2147483647) {
              printError(parent->getModule()->getSchemaname(), parent->getName().convertedValue,
                Mstring("Invalid unicode character."));
              TTCN3ModuleInventory::getInstance().incrNumErrors();
            }
            unsigned char group = (d >> 24) & 0xFF;
            unsigned char plane = (d >> 16) & 0xFF;
            unsigned char row = (d >> 8) & 0xFF;
            unsigned char cell = d & 0xFF;

            expstring_t res = mprintf("\\q{%d, %d, %d, %d}", group, plane, row, cell);
            value += res;
            Free(res);
            i = i + k;
          }
          // else fall through
        default: //just_copy:
          value += c;
          break;
      } // switch(c)
    } // next i
  } else {
    printWarning(parent->getModule()->getSchemaname(), parent->getName().convertedValue,
      Mstring("Pattern restriction is not supported on type '") + base + Mstring("'."));
    TTCN3ModuleInventory::getInstance().incrNumWarnings();
  }
}

void PatternType::printToFile(FILE * file) const {
  if (!modified || value.empty()) return;
  if (parent->getEnumeration().modified) return;

  fprintf(file, " (pattern \"%s\")", value.c_str());
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

EnumerationType::EnumerationType(SimpleType * a_simpleType)
: parent(a_simpleType)
, modified(false)
, facets()
, items_string()
, items_int()
, items_float()
, items_time()
, items_misc()
, variants()
, converted_facets() {
}

void EnumerationType::applyReference(const EnumerationType & other) {
  if (!modified) modified = other.modified;
  if ((other.parent->getXsdtype() == n_NOTSET && parent->getMode() != SimpleType::restrictionMode)
        || parent->getXsdtype() == n_simpleType) {
    for (List<Mstring>::iterator facet = other.facets.begin(); facet; facet = facet->Next) {
      facets.push_back(facet->Data);
    }
  }
}

void EnumerationType::applyFacets() // string types, integer types, float types, time types
{
  if (!modified) return;

  facets.remove_dups();

  const Mstring & base = parent->getBuiltInBase();

  if (isStringType(base)) // here length restriction is applicable
  {
    List<Mstring> text_variants;
    for (List<Mstring>::iterator facet = facets.begin(); facet; facet = facet->Next) {
      const LengthType & length = parent->getLength();
      if (length.lower <= facet->Data.size() && facet->Data.size() <= length.upper) {
        Mstring res, var;
        XSDName2TTCN3Name(facet->Data, items_string, enum_id_name, res, var);
        text_variants.push_back(var);
        converted_facets.push_back(res);
      }
    }
    text_variants.sort();
    for (List<Mstring>::iterator var = text_variants.end(); var; var = var->Prev) {
      variants.push_back(var->Data);
    }
  } else if (isIntegerType(base)) // here value restriction is applicable
  {
    for (List<Mstring>::iterator facet = facets.begin(); facet; facet = facet->Next) {
      int int_value = atoi(facet->Data.c_str());
      const ValueType & value = parent->getValue();
      if (value.lower <= int_value && int_value <= value.upper) {
        bool found = false;
        for (List<int>::iterator itemInt = items_int.begin(); itemInt; itemInt = itemInt->Next) {
          if (int_value == itemInt->Data) {
            found = true;
            break;
          }
        }
        if (!found) {
          items_int.push_back(int_value);
          expstring_t tmp = NULL;
          if (int_value < 0) {
            tmp = mputprintf(tmp, "int_%d", abs(int_value));
          } else {
            tmp = mputprintf(tmp, "int%d", int_value);
          }
          converted_facets.push_back(Mstring(tmp));
          Free(tmp);
        }

        if (variants.empty() || variants.back() != "\"useNumber\"") {
          variants.push_back(Mstring("\"useNumber\""));
        }
      }
    }
  } else if (isFloatType(base)) // here value restriction is applicable
  {
    for (List<Mstring>::iterator facet = facets.begin(); facet; facet = facet->Next) {
      double float_value = atof(facet->Data.c_str());
      const ValueType & value = parent->getValue();
      // Value uses DBL_MAX as infinity or -DBL_MAX as -infinity
      if ((value.lower <= float_value && float_value <= value.upper)
        || (facet->Data == "-INF" && value.lower == -DBL_MAX)
        || (facet->Data == "INF" && value.upper == DBL_MAX)
        || facet->Data == "NaN") {
        bool found = false;
        for (List<double>::iterator itemFloat = items_float.begin(); itemFloat; itemFloat = itemFloat->Next) {
          if (float_value == itemFloat->Data) {
            found = true;
            break;
          }
        }
        if (!found) {
          items_float.push_back(float_value);
          Mstring res = xmlFloat2TTCN3FloatStr(facet->Data);
          converted_facets.push_back(res);
        }
      }
    }
  } else if (isTimeType(base)) {
    List<Mstring> text_variants;
    for (List<Mstring>::iterator facet = facets.begin(); facet; facet = facet->Next) {
      Mstring res, var;
      XSDName2TTCN3Name(facet->Data, items_time, enum_id_name, res, var);
      text_variants.push_back(var);
      converted_facets.push_back(res);
    }
    text_variants.sort();
    for (List<Mstring>::iterator var = text_variants.end(); var; var = var->Prev) {
      variants.push_back(var->Data);
    }
  } else if (isAnyType(base)) {
  } else if (base.empty()) {
  } else {
    printWarning(parent->getModule()->getSchemaname(), parent->getName().convertedValue,
      Mstring("Enumeration restriction is not supported on type '") + base + Mstring("'."));
    TTCN3ModuleInventory::getInstance().incrNumWarnings();
    parent->setInvisible();
  }
}

void EnumerationType::sortFacets() {
  items_string.sort();
  items_int.sort();
  items_float.sort();
  items_time.sort();
}

void EnumerationType::printToFile(FILE * file, unsigned int indent_level) const {
  if (!modified) return;

  const Mstring & base = parent->getBuiltInBase();
  if (isStringType(base)) {
    for (QualifiedNames::iterator itemString = items_string.begin(); itemString; itemString = itemString->Next) {
      if (itemString != items_string.begin()) fputs(",\n", file);
      for (unsigned int l = 0; l != indent_level; ++l) fputs("\t", file);
      fprintf(file, "\t%s", itemString->Data.name.c_str());
    }
  } else if (isIntegerType(base)) {
    for (List<int>::iterator itemInt = items_int.begin(); itemInt; itemInt = itemInt->Next) {
      if (itemInt != items_int.begin()) fputs(",\n", file);
      for (unsigned int l = 0; l != indent_level; ++l) fputs("\t", file);
      if (itemInt->Data < 0) {
        fprintf(file, "\tint_%d(%d)", abs(itemInt->Data), itemInt->Data);
      } else {
        fprintf(file, "\tint%d(%d)", itemInt->Data, itemInt->Data);
      }
    }
  } else if (isFloatType(base)) {
    for (List<double>::iterator itemFloat = items_float.begin(); itemFloat; itemFloat = itemFloat->Next) {
      if (itemFloat != items_float.begin()) fputs(", ", file);

      if (itemFloat->Data == PLUS_INFINITY) {
        fprintf(file, "infinity");
      } else if (itemFloat->Data == MINUS_INFINITY) {
        fprintf(file, "-infinity");
      } else if (isnan(itemFloat->Data)) {
        fprintf(file, "not_a_number");
      } else {
        double intpart = 0;
        double fracpart = 0;
        fracpart = modf(itemFloat->Data, &intpart);
        if (fracpart == 0) {
          fprintf(file, "%lld.0", (long long int) (itemFloat->Data));
        } else {
          fprintf(file, "%g", itemFloat->Data);
        }
      }
    }
  } else if (isTimeType(base)) {
    for (QualifiedNames::iterator itemTime = items_time.begin(); itemTime; itemTime = itemTime->Next) {
      if (itemTime != items_time.begin()) fputs(",\n", file);
      for (unsigned int l = 0; l != indent_level; ++l) fputs("\t", file);
      fprintf(file, "\t%s", itemTime->Data.name.c_str());
    }
  }
}

void EnumerationType::insertVariants(){
  if(!modified) return;

  Mstring pre_connector = empty_string;
  if(parent->getMinOccurs() == 0 && parent->getMaxOccurs() == ULLONG_MAX){
    pre_connector = "([-]) ";
  }
  for(List<Mstring>::iterator var = variants.begin(); var; var = var->Next){
    parent->addVariant(V_onlyValue, pre_connector + var->Data);
  }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * */

WhitespaceType::WhitespaceType(SimpleType * a_simpleType)
: p_parent(a_simpleType)
, modified(false)
, facet()
, value() {
}

void WhitespaceType::applyReference(const WhitespaceType & other) {
  if (!modified) modified = other.modified;
  if (facet.empty()) facet = other.facet;
}

void WhitespaceType::applyFacet() // only for string types: string, normalizedString, token, Name, NCName, language
{
  if (!modified) return;

  const Mstring & base = p_parent->getBuiltInBase();
  if (base == "string" || base == "normalizedString" || base == "token" || base == "language" ||
    base == "Name" || base == "NCName" || isAnyType(base) || base.empty()) {
    p_parent->addVariant(V_whiteSpace, facet);
  } else {
    printWarning(p_parent->getModule()->getSchemaname(), p_parent->getName().convertedValue,
      Mstring("Facet 'whiteSpace' is not applicable for type '") + base + Mstring("'."));
    TTCN3ModuleInventory::getInstance().incrNumWarnings();
  }
}

ValueType::ValueType(SimpleType * a_simpleType)
: parent(a_simpleType)
, modified(false)
, facet_minInclusive(-DBL_MAX)
, facet_maxInclusive(DBL_MAX)
, facet_minExclusive(-DBL_MAX)
, facet_maxExclusive(DBL_MAX)
, facet_totalDigits(-1)
, lower(-DBL_MAX)
, upper(DBL_MAX)
, lowerExclusive(false)
, upperExclusive(false)
, not_a_number(false)
, fixed_value()
, default_value()
, items_with_value() {
}

void ValueType::applyReference(const ValueType & other) {
  if (!modified) {
    modified = other.modified;
  }
  if (not_a_number) {
    return;
  }
  if (other.not_a_number) not_a_number = true;
  if (other.facet_minInclusive > facet_minInclusive) facet_minInclusive = other.facet_minInclusive;
  if (other.facet_maxInclusive < facet_maxInclusive) facet_maxInclusive = other.facet_maxInclusive;
  if (other.facet_minExclusive > facet_minExclusive) facet_minExclusive = other.facet_minExclusive;
  if (other.facet_maxExclusive < facet_maxExclusive) facet_maxExclusive = other.facet_maxExclusive;
  if (other.upperExclusive) upperExclusive = other.upperExclusive;
  if (other.lowerExclusive) lowerExclusive = other.lowerExclusive;
  //-1 in case when it is not modified
  if (other.facet_totalDigits < facet_totalDigits || facet_totalDigits == -1) facet_totalDigits = other.facet_totalDigits;
  if (!other.default_value.empty()) {
    default_value = other.default_value;
    parent->addVariant(V_defaultForEmpty, default_value);
  }
  if (!other.fixed_value.empty()) {
    fixed_value = other.fixed_value;
    parent->addVariant(V_defaultForEmpty, fixed_value);
  }
}

void ValueType::applyFacets() // only for integer and float types
{
  if (!modified) {
    return;
  }

  if (not_a_number) {
    return;
  }

  const Mstring & base = parent->getBuiltInBase();
  /*
   * Setting of default value range of built-in types
   */
  if (base == "positiveInteger") {
    lower = 1;
  } else if (base == "nonPositiveInteger") {
    upper = 0;
  } else if (base == "negativeInteger") {
    upper = -1;
  } else if (base == "nonNegativeInteger") {
    lower = 0;
  } else if (base == "unsignedLong") {
    lower = 0;
    upper = ULLONG_MAX;
  } else if (base == "int") {
    lower = INT_MIN;
    upper = INT_MAX;
  } else if (base == "unsignedInt") {
    lower = 0;
    upper = UINT_MAX;
  } else if (base == "short") {
    lower = SHRT_MIN;
    upper = SHRT_MAX;
  } else if (base == "unsignedShort") {
    lower = 0;
    upper = USHRT_MAX;
  } else if (base == "byte") {
    lower = CHAR_MIN;
    upper = CHAR_MAX;
  } else if (base == "unsignedByte") {
    lower = 0;
    upper = UCHAR_MAX;
  }

  if (isIntegerType(base)) {
    if (facet_minInclusive != -DBL_MAX && facet_minInclusive > lower) lower = facet_minInclusive;
    if (facet_maxInclusive != DBL_MAX && upper > facet_maxInclusive) upper = facet_maxInclusive;
    if (facet_minExclusive != -DBL_MAX && lower < facet_minExclusive) lower = facet_minExclusive;
    if (facet_maxExclusive != DBL_MAX && upper > facet_maxExclusive) upper = facet_maxExclusive;
  } else if (isFloatType(base)) {
    if (facet_minInclusive != -DBL_MAX && lower < facet_minInclusive) lower = facet_minInclusive;
    if (facet_maxInclusive != DBL_MAX && upper > facet_maxInclusive) upper = facet_maxInclusive;
    if (facet_minExclusive != -DBL_MAX && lower < facet_minExclusive) lower = facet_minExclusive;
    if (facet_maxExclusive != DBL_MAX && upper > facet_maxExclusive) upper = facet_maxExclusive;
  } else if (isAnyType(base) || isTimeType(base) || isBooleanType(base)) {
  } else if (isStringType(base) && (
    base.getValueWithoutPrefix(':') != "hexBinary" && base.getValueWithoutPrefix(':') != "base64Binary")) {
  } else if (base.empty()) {
  } else {
    printWarning(parent->getModule()->getSchemaname(), parent->getName().convertedValue,
      Mstring("Value restriction is not supported on type '") + base + Mstring("'."));
    TTCN3ModuleInventory::getInstance().incrNumWarnings();
  }

  // totalDigits facet is only for integer types and decimal
  if (facet_totalDigits > 0) // if this facet is used
  {
    double r = pow(10.0, facet_totalDigits);

    if (base == "integer") {
      lower = static_cast<int>( -(r - 1) );
      upper = static_cast<int>(r - 1);
    } else if (base == "positiveInteger") {
      lower = 1;
      upper = static_cast<int>(r - 1);
    } else if (base == "nonPositiveInteger") {
      lower = static_cast<int>( -(r - 1) );
      upper = 0;
    } else if (base == "negativeInteger") {
      lower = static_cast<int>( -(r - 1) );
      upper = -1;
    } else if (base == "nonNegativeInteger") {
      lower = 0;
      upper = static_cast<int>(r - 1);
    } else if (base == "long" ||
      base == "unsignedLong" ||
      base == "int" ||
      base == "unsignedInt" ||
      base == "short" ||
      base == "unsignedShort" ||
      base == "byte" ||
      base == "unsignedByte") {
      lower = static_cast<int>( -(r - 1) );
      upper = static_cast<int>(r - 1);
    } else if (base == "decimal") {
      lower = static_cast<int>( -(r - 1) );
      upper = static_cast<int>(r - 1);
    } else {
      printWarning(parent->getModule()->getSchemaname(), parent->getName().convertedValue,
        Mstring("Facet 'totalDigits' is not applicable for type '") + base + Mstring("'."));
      TTCN3ModuleInventory::getInstance().incrNumWarnings();
    }
  }
  items_with_value.sort();
}

void ValueType::printToFile(FILE * file) const {
  if (!modified) return;
  if (parent->getEnumeration().modified) return;

  if (not_a_number) {
    fprintf(file, " ( not_a_number )");
    return;
  }
  if (!fixed_value.empty()) {
    if (parent->getConstantDefaultForEmpty() != NULL) {
      // Insert the constant's name in the TTCN type restriction.
      TTCN3Module * mod = parent->parent != NULL ? parent->parent->getModule() :
                          parent->getModule();
      Mstring val = parent->getConstantDefaultForEmpty()->getAlterego()->getConstantName(mod);
      fprintf(file, " (%s)", val.c_str());
    } else {
      //Base64binary and hexbyte does not supported
      Mstring type;
      if(isBuiltInType(parent->getType().originalValueWoPrefix)){
        type = parent->getType().originalValueWoPrefix; 
      }else {
        type = getPrefixByNameSpace(parent, parent->getReference().get_uri()) + Mstring(":") + parent->getReference().get_val();
      }
      if(!isBuiltInType(type)){
        type = findBuiltInType(parent, type);
      }
      if (isStringType(type) || isTimeType(type) || isQNameType(type) || isAnyType(type)) {
        const Mstring& name = type.getValueWithoutPrefix(':');
        if (name != "hexBinary" && name != "base64Binary") {
          fprintf(file, " (\"%s\")", fixed_value.c_str());
        }
      } else if (isBooleanType(type)) {
        Mstring val;
        if (fixed_value == "1") {
          val = "true";
        } else if (fixed_value == "0") {
          val = "false";
        } else {
          val = fixed_value;
        }
        fprintf(file, " (%s)", val.c_str());
      } else if (isFloatType(type)) {
        Mstring val = xmlFloat2TTCN3FloatStr(fixed_value);
        fprintf(file, " (%s)", val.c_str());
      } else {
        fprintf(file, " (%s)", fixed_value.c_str());
      }
    }
    return;
  }
  if (!items_with_value.empty()) {
    fputs(" (\n", file);
    for (List<Mstring>::iterator itemWithValue = items_with_value.begin(); itemWithValue; itemWithValue = itemWithValue->Next) {
      fprintf(file, "\t%s", itemWithValue->Data.c_str());
      if (itemWithValue != items_with_value.end()) {
        fputs(",\n", file);
      } else {
        fputs("\n", file);
      }
    }
    fputc(')', file);
    return;
  }

  if (lower == -DBL_MAX && upper == DBL_MAX) return;

  fputs(" (", file);

  if (isIntegerType(parent->getBuiltInBase())) {
    if (lowerExclusive) {
      fputc('!', file);
    }

    if (lower == -DBL_MAX) {
      fputs("-infinity", file);
    } else if (lower < 0) {
      long double temp_lower = -lower;
      fprintf(file, "-%.0Lf", temp_lower);
    } else {
      fprintf(file, "%.0Lf", lower);
    }

    fputs(" .. ", file);
    if (upperExclusive) {
      fputc('!', file);
    }

    if (upper == DBL_MAX) {
      fputs("infinity", file);
    } else if (upper < 0) {
      long double temp_upper = -upper;
      fprintf(file, "-%.0Lf", temp_upper);
    } else {
      fprintf(file, "%.0Lf", upper);
    }
  } else if (isFloatType(parent->getBuiltInBase())) {
    if (lowerExclusive) {
      fputc('!', file);
    }

    if (lower == -DBL_MAX) {
      fputs("-infinity", file);
    } else {
      double intpart = 0;
      double fracpart = 0;
      fracpart = modf(lower, &intpart);
      if (fracpart == 0) {
        fprintf(file, "%.1Lf", lower);
      } else {
        fprintf(file, "%Lg", lower);
      }
    }

    fputs(" .. ", file);
    if (upperExclusive) {
      fputc('!', file);
    }

    if (upper == DBL_MAX) {
      fputs("infinity", file);
    } else {
      double intpart = 0;
      double fracpart = 0;
      fracpart = modf(upper, &intpart);
      if (fracpart == 0) {
        fprintf(file, "%.1Lf", upper);
      } else {
        fprintf(file, "%Lg", upper);
      }
    }
  }

  fputc(')', file);
}

