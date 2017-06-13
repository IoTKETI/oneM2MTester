/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Beres, Szabolcs
 *   Godar, Marton
 *   Ormandi, Matyas
 *   Raduly, Csaba
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
#include "ComplexType.hh"

#include "GeneralFunctions.hh"
#include "XMLParser.hh"
#include "TTCN3Module.hh"
#include "TTCN3ModuleInventory.hh"
#include "Annotation.hh"

#include <assert.h>

ComplexType::ComplexType(XMLParser * a_parser, TTCN3Module * a_module, ConstructType a_construct)
: SimpleType(a_parser, a_module, a_construct)
, top(true)
, nillable(false)
, enumerated(false)
, embed(false)
, with_union(false)
, first_child(false)
, fromAll(false)
, listPrint(false)
, listMinOccurs(1)
, listMaxOccurs(1)
, max_alt(0)
, skipback(0)
, lastType()
, actualPath(empty_string)
, actfield(this)
, nameDep(NULL)
, nillable_field(NULL)
, basefield(NULL)
, cmode(CT_undefined_mode)
, resolved(No)
, parentTypeSubsGroup(NULL)
, complexfields()
, attribfields()
, enumfields()
, tagNames() {
  xsdtype = n_complexType;
}

ComplexType::ComplexType(ComplexType & other)
: SimpleType(other)
, top(other.top)
, nillable(other.nillable)
, enumerated(other.enumerated)
, embed(other.embed)
, with_union(other.with_union)
, first_child(other.first_child)
, fromAll(other.fromAll)
, listPrint(other.listPrint)
, listMinOccurs(other.listMinOccurs)
, listMaxOccurs(other.listMaxOccurs)
, max_alt(other.max_alt)
, skipback(other.skipback)
, lastType(other.lastType)
, actualPath(other.actualPath)
, actfield(this)
, nameDep(other.nameDep)
, nillable_field(NULL)
, basefield(NULL)
, cmode(other.cmode)
, resolved(other.resolved)
, parentTypeSubsGroup(other.parentTypeSubsGroup) {
  type.originalValueWoPrefix = other.type.originalValueWoPrefix;
  for (List<AttributeType*>::iterator attr = other.attribfields.begin(); attr; attr = attr->Next) {
    attribfields.push_back(new AttributeType(*attr->Data));
    attribfields.back()->parent = this;
  }

  for (List<ComplexType*>::iterator field = other.complexfields.begin(); field; field = field->Next) {
    complexfields.push_back(new ComplexType(*field->Data));
    complexfields.back()->parent = this;
    if(field->Data == other.basefield){
      basefield = complexfields.back();
    }else if(field->Data == other.nillable_field){
      nillable_field = complexfields.back();
    }
  }

  if (other.nameDep != NULL) {
    SimpleType* dep = other.nameDep;
    if(dep->getSubstitution() != NULL){
      dep->getSubstitution()->addToNameDepList(this);
      nameDep = dep->getSubstitution();
    }else {
      other.nameDep->addToNameDepList(this);
    }
  }
}

ComplexType::ComplexType(ComplexType * other)
: SimpleType(other->getParser(), other->getModule(), c_unknown)
, top(false)
, nillable(false)
, enumerated(false)
, embed(false)
, with_union(false)
, first_child(false)
, fromAll(false)
, listPrint(false)
, listMinOccurs(1)
, listMaxOccurs(1)
, max_alt(0)
, skipback(0)
, lastType()
, actualPath(empty_string)
, actfield(this)
, nameDep(NULL)
, nillable_field(NULL)
, basefield(NULL)
, cmode(CT_undefined_mode)
, resolved(No)
, parentTypeSubsGroup(NULL)
, complexfields()
, attribfields()
, enumfields()
, tagNames() {
  xsdtype = n_complexType;
  parent = other;
  outside_reference = ReferenceData();
}

ComplexType::ComplexType(const SimpleType & other, CT_fromST c)
: SimpleType(other)
, top(true)
, nillable(false)
, enumerated(false)
, embed(false)
, with_union(false)
, first_child(false)
, fromAll(false)
, listPrint(false)
, listMinOccurs(1)
, listMaxOccurs(1)
, max_alt(0)
, skipback(0)
, lastType()
, actualPath(empty_string)
, actfield(this)
, nameDep(NULL)
, nillable_field(NULL)
, basefield(NULL)
, cmode(CT_simpletype_mode)
, resolved(No)
, parentTypeSubsGroup(NULL)
, complexfields()
, attribfields()
, enumfields()
, tagNames() {

  if(c != fromTagSubstitution && c != fromTypeSubstitution){
    module->replaceLastMainType(this);
    module->setActualXsdConstruct(c_complexType);
  }
  construct = c_complexType;

  switch (c) {
    case fromTagUnion:
      type.upload(Mstring("union"), false);
      with_union = true;
      xsdtype = n_union;
      break;
    case fromTagNillable:
      addVariant(V_useNil);
      type.upload(Mstring("record"), false);
      break;
    case fromTagComplexType:
      type.upload(Mstring("record"), false);
      xsdtype = n_complexType;
      break;
    case fromTagSubstitution:
      type.upload(Mstring("union"), false);
      name.upload(getName().originalValueWoPrefix + Mstring("_group"));
      xsdtype = n_union;
      subsGroup = this;
      variant.clear();
      hidden_variant.clear();
      enumeration.modified = false;
      value.modified = false;
      pattern.modified = false;
      length.modified = false;
      whitespace.modified = false;
      break;
    case fromTypeSubstitution:
      type.upload(Mstring("union"), false);
      name.upload(getName().originalValueWoPrefix + Mstring("_derivations"));
      xsdtype = n_union;
      substitutionGroup = empty_string;
      typeSubsGroup = this;
      variant.clear();
      hidden_variant.clear();
      enumeration.modified = false;
      value.modified = false;
      pattern.modified = false;
      length.modified = false;
      whitespace.modified = false;
  }
}

ComplexType::~ComplexType() {
  for (List<ComplexType*>::iterator field = complexfields.begin(); field; field = field->Next) {
    delete field->Data;
    field->Data = NULL;
  }
  complexfields.clear();

  for (List<AttributeType*>::iterator field = attribfields.begin(); field; field = field->Next) {
    delete field->Data;
    field->Data = NULL;
  }
  attribfields.clear();
}

void ComplexType::loadWithValues() {
  //Find the last field where the tag is found
  if (this != actfield) {
    actfield->loadWithValues();
    return;
  }
  
  const XMLParser::TagAttributes & atts = parser->getActualTagAttributes();
  
  switch (parser->getActualTagName()) {
    case n_sequence:
      if (!top && xsdtype != n_sequence && xsdtype != n_complexType && xsdtype != n_extension && xsdtype != n_restriction && xsdtype != n_element) {
        //Create new record
        ComplexType * rec = new ComplexType(this);
        rec->type.upload(Mstring("record"), false);
        rec->name.upload(Mstring("sequence"));
        rec->addVariant(V_untagged);
        rec->setMinMaxOccurs(atts.minOccurs, atts.maxOccurs);
        rec->setXsdtype(n_sequence);
        complexfields.push_back(rec);
        actfield = rec;
      } else {
        //Do not create new record, it is an embedded sequence
        if (xsdtype == n_sequence && atts.minOccurs == 1 && atts.maxOccurs == 1) {
          skipback += 1;
        }
        type.upload(Mstring("record"), false);
        xsdtype = n_sequence;
        setMinMaxOccurs(atts.minOccurs, atts.maxOccurs);
      }
      break;
    case n_choice:
      if (!top || xsdtype != n_group) {
        //Create new union field
        ComplexType * choice = new ComplexType(this);
        choice->type.upload(Mstring("union"), false);
        choice->name.upload(Mstring("choice"));
        choice->setXsdtype(n_choice);
        choice->addVariant(V_untagged);
        choice->setMinMaxOccurs(atts.minOccurs, atts.maxOccurs);
        actfield = choice;
        complexfields.push_back(choice);
      } else {
        xsdtype = n_choice;
        type.upload(Mstring("union"), false);
      }
      break;
    case n_all:
    {
      //Create the record of enumerated field
      xsdtype = n_all;
      ComplexType * enumField = new ComplexType(this);
      enumField->setTypeValue(Mstring("enumerated"));
      enumField->setNameValue(Mstring("order"));
      enumField->setBuiltInBase(Mstring("string"));
      enumField->enumerated = true;
      enumField->setMinMaxOccurs(0, ULLONG_MAX, false);
      setMinMaxOccurs(atts.minOccurs, atts.maxOccurs);
      addVariant(V_useOrder);
      complexfields.push_back(enumField);
      if (atts.minOccurs == 0) {
        isOptional = true;
      }
      break;
    }
    case n_restriction:
      mode = restrictionMode;
      //If it is an xsd:union then call SimpleType::loadWithValues
      if (parent != NULL && parent->with_union && parent->hasVariant(Mstring("useUnion"))) {
        SimpleType::loadWithValues();
        break;
      }
      if (cmode == CT_simpletype_mode) {
        //if it is from a SimpleType, then create a base field
        ComplexType * f = new ComplexType(this);
        f->name.upload(Mstring("base"));
        f->type.upload(atts.base);
        f->setReference(atts.base);
        f->addVariant(V_untagged);
        f->mode = restrictionMode;
        complexfields.push_back(f);
        basefield = f;
        actfield = f;
        
        // If it is a restriction of a list, then no new basefield will be 
        // present, to we apply the references to the parent.
        if(parent != NULL && parent->inList) {
          parent->applyReference(*f, true);
        }
      } else if (cmode == CT_complextype_mode) {
        setReference(atts.base);
        xsdtype = n_restriction;
      }
      break;
    case n_extension:
      mode = extensionMode;
      if (cmode == CT_simpletype_mode) {
        //if it is from a SimpleType, then create a base field
        ComplexType * f = new ComplexType(this);
        f->name.upload(Mstring("base"));
        f->type.upload(atts.base);
        f->setReference(atts.base);
        f->addVariant(V_untagged);
        f->mode = extensionMode;
        complexfields.push_back(f);
        basefield = f;
        actfield = f;
      } else if (cmode == CT_complextype_mode) {
        setReference(atts.base);
        xsdtype = n_extension;
      }
      break;
    case n_element:
    {
      if (atts.nillable) {
        if(cmode == CT_simpletype_mode){
          //If a simple top level element is nillable
          ComplexType * nilrec = new ComplexType(this);
          if (atts.type.empty()) {
            nilrec->type.upload(Mstring("record"), false);
          } else {
            nilrec->type.upload(atts.type);
          }
          nilrec->name.upload(Mstring("content"));
          nilrec->isOptional = true;
          nilrec->nillable = true;
          setMinMaxOccurs(atts.minOccurs, atts.maxOccurs);
          complexfields.push_back(nilrec);
          type.upload(Mstring("record"), false);
          name.upload(atts.name);
          actfield = nilrec;
          nillable_field = nilrec;
        } else {
          //From a complexType element is nillable
          ComplexType * record = new ComplexType(this);
          ComplexType * nilrec = new ComplexType(record);
          if (atts.type.empty()) {
            nilrec->type.upload(Mstring("record"), false);
          } else {
            nilrec->type.upload(atts.type);
          }
          record->name.upload(atts.name);
          record->type.upload(Mstring("record"), false);
          record->complexfields.push_back(nilrec);
          record->addVariant(V_useNil);
          record->nillable_field = nilrec;
          record->setMinMaxOccurs(atts.minOccurs, atts.maxOccurs);

          nilrec->name.upload(Mstring("content"));
          nilrec->nillable = true;
          nilrec->isOptional = true;
          nilrec->tagNames.push_back(parser->getActualTagName());
          complexfields.push_back(record);
          actfield = nilrec;
        }
      }else {
        //It is a simple element
        ComplexType* c = new ComplexType(this);
        c->setXsdtype(n_element);
        c->type.upload(atts.type);
        c->name.upload(atts.name);
        c->setReference(atts.type);
        c->setMinMaxOccurs(atts.minOccurs, atts.maxOccurs);
        c->applyDefaultAttribute(atts.default_);
        c->applyFixedAttribute(atts.fixed);
        c->setElementFormAs(atts.form);
        if (atts.ref.empty()) {
          c->setReference(atts.type);
        } else {
          c->applyRefAttribute(atts.ref);
          c->name.upload(atts.ref.getValueWithoutPrefix(':'));
          c->type.upload(atts.ref);
        }
        c->applySubstitionGroupAttribute(atts.substitionGroup);
        c->applyBlockAttribute(atts.block);
        actfield = c;

        //Inside all have some special conditions
        if (xsdtype == n_all) {
          if (atts.minOccurs > 1) {
            printError(getModule()->getSchemaname(), name.convertedValue,
              Mstring("Inside <all>, minOccurs must be 0 or 1"));
            TTCN3ModuleInventory::incrNumErrors();
          }
          if (atts.maxOccurs != 1) {
            printError(getModule()->getSchemaname(), name.convertedValue,
              Mstring("Inside <all>, maxOccurs must be 1"));
            TTCN3ModuleInventory::incrNumErrors();
          }
          c->fromAll = true;
          complexfields.push_back(c);
          if (isOptional) {
            c->isOptional = true;
          }
        } else {
          complexfields.push_back(c);
        }
      }
      break;
    }
    case n_attribute:
    {
      AttributeType * attribute = new AttributeType(this);
      attribute->addVariant(V_attribute);
      attribute->applyMinMaxOccursAttribute(0, 1);
      attribute->setXsdtype(n_attribute);
      attribute->setUseVal(atts.use);
      attribute->setAttributeFormAs(atts.form);
      lastType = n_attribute;
      if (atts.ref.empty()) {
        attribute->setNameOfField(atts.name);
        attribute->setTypeOfField(atts.type);
        attribute->setReference(atts.type, true);
      } else {
        attribute->applyRefAttribute(atts.ref);
      }
      attribute->applyDefaultAttribute(atts.default_);
      attribute->applyFixedAttribute(atts.fixed);
      actfield = attribute;
      
      //In case of nillable parent it is difficult...
      if (nillable && parent != NULL) {
        parent->attribfields.push_back(attribute);
        attribute->parent = parent;
      } else if (nillable && !complexfields.empty() && parent == NULL) {
        complexfields.back()->attribfields.push_back(attribute);
      } else if (parent != NULL && (parent->mode == extensionMode || parent->mode == restrictionMode) && name.convertedValue == Mstring("base")) {
        parent->attribfields.push_back(attribute);
        attribute->parent = parent;
      } else {
        attribfields.push_back(attribute);
      }
      break;
    }
    case n_any:
    {
      ComplexType * any = new ComplexType(this);
      any->name.upload(Mstring("elem"));
      any->type.upload(Mstring("string"), false);
      any->applyNamespaceAttribute(V_anyElement, atts.namespace_);
      any->setMinMaxOccurs(atts.minOccurs, atts.maxOccurs);
      any->setXsdtype(n_any);
      complexfields.push_back(any);
      break;
    }
    case n_anyAttribute:
    {
      AttributeType * anyattr = new AttributeType(this);
      anyattr->setXsdtype(n_anyAttribute);
      anyattr->setNameOfField(Mstring("attr"));
      anyattr->setTypeValue(Mstring("string"));
      anyattr->setToAnyAttribute();
      anyattr->applyMinMaxOccursAttribute(0, ULLONG_MAX);
      anyattr->addNameSpaceAttribute(atts.namespace_);
      actfield = anyattr;

      //In case of nillable parent it is difficult...
      if (nillable && parent != NULL) {
        parent->attribfields.push_back(anyattr);
        anyattr->parent = parent;
      } else if (nillable && !complexfields.empty() && parent == NULL) {
        complexfields.back()->attribfields.push_back(anyattr);
      } else if (parent != NULL && (parent->mode == extensionMode || parent->mode == restrictionMode) && name.convertedValue == Mstring("base")) {
        parent->attribfields.push_back(anyattr);
        anyattr->parent = parent;
      } else {
        attribfields.push_back(anyattr);
      }
      break;
    }
    case n_attributeGroup:
      if (!atts.ref.empty()) {
        ComplexType * g = new ComplexType(this);
        g->setXsdtype(n_attributeGroup);
        g->setReference(atts.ref);
        complexfields.push_back(g);
        actfield = g;
      } else {
        xsdtype = n_attributeGroup;
        name.upload(Mstring(atts.name));
        setInvisible();
      }
      break;
    case n_group:
      if (atts.ref.empty()) {
        //It is a definition
        xsdtype = n_group;
        name.upload(atts.name);
      } else {
        //It is a reference
        ComplexType* group = new ComplexType(this);
        group->setXsdtype(n_group);
        group->name.upload(atts.name);
        group->setReference(Mstring(atts.ref));
        group->setMinMaxOccurs(atts.minOccurs, atts.maxOccurs);
        complexfields.push_back(group);
        actfield = group;
      }
      break;
    case n_union:
    {
      with_union = true;
      xsdtype = n_union;
      type.upload(Mstring("union"), false);
      addVariant(V_useUnion);
      if (!atts.memberTypes.empty()) {
        List<Mstring> types;
        //Get the union values
        expstring_t valueToSplitIntoTokens = mcopystr(atts.memberTypes.c_str());
        char * token;
        token = strtok(valueToSplitIntoTokens, " ");
        while (token != NULL) {
          types.push_back(Mstring(token));
          token = strtok(NULL, " ");
        }
        Free(valueToSplitIntoTokens);
        
        //Create the union elements and push into the container
        for (List<Mstring>::iterator memberType = types.begin(); memberType; memberType = memberType->Next) {
          Mstring tmp_name = memberType->Data.getValueWithoutPrefix(':');
          ComplexType * f = new ComplexType(this);
          f->name.upload(tmp_name);
          f->type.upload(memberType->Data);
          f->setXsdtype(n_simpleType);
          f->setReference(memberType->Data);
          complexfields.push_back(f);
        }
      }
      break;
    }
    case n_simpleType:
    case n_simpleContent:
    {
      xsdtype = parser->getActualTagName();
      cmode = CT_simpletype_mode;
      if (with_union && hasVariant(Mstring("useUnion"))) {
        Mstring fieldname;
        if (max_alt == 0) {
          fieldname = Mstring("alt_");
        } else {
          expstring_t new_name = mprintf("alt_%d", max_alt);
          fieldname = new_name;
          Free(new_name);
        }
        max_alt++;
        ComplexType * field = new ComplexType(this);
        field->name.upload(fieldname);
        field->setXsdtype(n_simpleType);
        field->addVariant(V_nameAs, empty_string, true);
        complexfields.push_back(field);
        actfield = field;
      }
      break;
    }
    case n_complexType:
      name.upload(atts.name);
      type.upload(Mstring("record"), false);
      applyAbstractAttribute(atts.abstract);
      applySubstitionGroupAttribute(atts.substitionGroup);
      applyBlockAttribute(atts.block);
      // fall through
    case n_complexContent:
      tagNames.push_back(parser->getActualTagName());
      cmode = CT_complextype_mode;
      if (atts.mixed) {
        ComplexType * mixed = new ComplexType(this);
        mixed->name.upload(Mstring("embed_values"));
        mixed->type.upload(Mstring("string"), false);
        mixed->setMinMaxOccurs(0, ULLONG_MAX, false);
        mixed->embed = true;
        complexfields.push_back(mixed);
        addVariant(V_embedValues);
      }
      break;
    case n_list:
      if (parent != NULL && parent->basefield == this) {
        if (parent->getMaxOccurs() == 1) { // optional or minOccurs = maxOccurs = 1
          if (parent->getMinOccurs() == 0) {
            parent->isOptional = true;
            if (parent->parent != NULL && parent->parent->getXsdtype() == n_choice) {
              parent->listPrint = true;
              parent->listMinOccurs = parent->getMinOccurs();
              parent->listMaxOccurs = parent->getMaxOccurs();
            }
          }
        } else if (parent->parent != NULL){
          parent->listPrint = true;
          parent->listMinOccurs = parent->getMinOccurs();
          parent->listMaxOccurs = parent->getMaxOccurs();
        }
        parent->basefield = NULL;
        parent->SimpleType::loadWithValues();
        setInvisible();
      } else if(parent != NULL) {
        if (getMaxOccurs() == 1) { // optional or minOccurs = maxOccurs = 1
          if (getMinOccurs() == 0) {
            isOptional = true;
            if (parent->getXsdtype() == n_choice) {
              listPrint = true;
              listMinOccurs = getMinOccurs();
              listMaxOccurs = getMaxOccurs();
            }
          }
        } else {
          listPrint = true;
          listMinOccurs = getMinOccurs();
          listMaxOccurs = getMaxOccurs();
        }
        SimpleType::loadWithValues();
      }
      break;
    case n_length:
    case n_minLength:
    case n_maxLength:
    case n_pattern:
    case n_enumeration:
    case n_whiteSpace:
    case n_minInclusive:
    case n_maxInclusive:
    case n_minExclusive:
    case n_maxExclusive:
    case n_totalDigits:
    case n_fractionDigits:
      SimpleType::loadWithValues();
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

// called from endelementHandler
void ComplexType::modifyValues() {
  if (this != actfield) {
    actfield->modifyValues();
    return;
  }
  if (xsdtype == n_sequence) {
    skipback = skipback - 1;
  }

  if ( parent != NULL && 
      (xsdtype == n_element || 
       xsdtype == n_complexType || 
       xsdtype == n_complexContent || 
       xsdtype == n_all || 
       xsdtype == n_attribute || 
       xsdtype == n_anyAttribute ||
       xsdtype == n_choice || 
       xsdtype == n_group || 
       xsdtype == n_attributeGroup || 
       xsdtype == n_extension || 
       xsdtype == n_restriction || 
       (xsdtype == n_simpleType && !inList) || 
       xsdtype == n_simpleContent ||
       (xsdtype == n_sequence && skipback < 0)
      )) {
    if (!tagNames.empty() && tagNames.back() == parser->getParentTagName()) {
      if (nillable && tagNames.back() == n_element) {
        parent->modifyValues();
      }
      tagNames.pop_back();
    } else if (tagNames.empty()) {
      parent->actfield = parent;
      parent->lastType = xsdtype;
    }
  }
  if (xsdtype == n_simpleType) {
    inList = false;
  }
}

void ComplexType::modifyList() {
  if (this != actfield) {
    ((SimpleType*)actfield)->modifyList();
    return;
  }
  if (!inList && mode == listMode && parent != NULL) {
    parent->actfield = parent;
    parent->lastType = xsdtype;
  }
}

void ComplexType::referenceResolving() {
  if (resolved != No) return; // nothing to do
  if(this == subsGroup){
    resolved = Yes;
    return;
  }
  resolved = InProgress;
  for (List<ComplexType*>::iterator ct = complexfields.begin(); ct; ct = ct->Next) {
    // Referenece resolving of ComplexTypes
    ct->Data->referenceResolving();
  }
  for (List<AttributeType*>::iterator attr = attribfields.begin(); attr; attr = attr->Next) {
    //Reference resolving for Attributes
    resolveAttribute(attr->Data);
  }
  
  reference_resolving_funtion();
  
  if(!substitutionGroup.empty()){
    addToSubstitutions();
  }
  resolved = Yes;
}

void ComplexType::reference_resolving_funtion() {
  //Every child element references are resolved here.
  if (outside_reference.empty() && basefield == NULL) {
    //Its not in the resolveElement function because we need the built in type
    //reference too, and then the outside_reference is empty.
    if(xsdtype == n_element){
      collectElementTypes(NULL, NULL);
    }
    return;
  }

  SimpleType * st = (SimpleType*) TTCN3ModuleInventory::getInstance().lookup(this, want_BOTH);
  if (st == NULL && basefield == NULL) {
    printError(module->getSchemaname(), name.convertedValue,
      "Reference for a non-defined type: " + getReference().repr());
    TTCN3ModuleInventory::getInstance().incrNumErrors();
    outside_reference.set_resolved(NULL);
    return;
  }

  resolveAttributeGroup(st);

  resolveGroup(st);

  resolveElement(st);

  resolveSimpleTypeExtension();

  resolveSimpleTypeRestriction();

  resolveComplexTypeExtension();

  resolveComplexTypeRestriction();

  resolveUnion(st);

  addToTypeSubstitutions();

}

void ComplexType::setParent(ComplexType * par, SimpleType * child) {
  child->parent = par;
}

void ComplexType::applyReference(const SimpleType & other, const bool on_attributes) {
  type.convertedValue = other.getType().convertedValue;
  type.originalValueWoPrefix = other.getType().convertedValue.getValueWithoutPrefix(':');

  if (other.getMinOccurs() > getMinOccurs() ||
      other.getMaxOccurs() < getMaxOccurs()) {
    if (!on_attributes) {
      expstring_t temp = memptystr();
      temp = mputprintf(
        temp,
        "The occurrence range (%llu .. %llu) of the element (%s) is not compatible "
        "with the occurrence range (%llu .. %llu) of the referenced element.",
        getMinOccurs(),
        getMaxOccurs(),
        name.originalValueWoPrefix.c_str(),
        other.getMinOccurs(),
        other.getMaxOccurs());
      printError(module->getSchemaname(), parent->getName().originalValueWoPrefix,
        Mstring(temp));
      Free(temp);
      TTCN3ModuleInventory::getInstance().incrNumErrors();
    }
  } else {
    setMinOccurs(llmax(getMinOccurs(), other.getMinOccurs()));
    setMaxOccurs(llmin(getMaxOccurs(), other.getMaxOccurs()));
  }

  for (List<Mstring>::iterator var = other.getVariantRef().begin(); var; var = var->Next) {
    bool found = false;
    for (List<Mstring>::iterator var1 = variant.begin(); var1; var1 = var1->Next) {
      if (var->Data == var1->Data) {
        found = true;
        break;
      }
    }
    if (!found) {
      variant.push_back(var->Data);
      variant_ref.push_back(var->Data);
    }
  }

  builtInBase = other.getBuiltInBase();
  
  length.applyReference(other.getLength());
  pattern.applyReference(other.getPattern());
  enumeration.applyReference(other.getEnumeration());
  whitespace.applyReference(other.getWhitespace());
  value.applyReference(other.getValue());
}

void ComplexType::nameConversion(NameConversionMode conversion_mode, const List<NamespaceType> & ns) {
  if(!visible) return;
  switch (conversion_mode) {
    case nameMode:
      nameConversion_names(ns);
      break;
    case typeMode:
      nameConversion_types(ns);
      break;
    case fieldMode:
      nameConversion_fields(ns);
      break;
  }
}

void ComplexType::nameConversion_names(const List<NamespaceType> &) {
  Mstring res, var(module->getTargetNamespace());
  XSDName2TTCN3Name(name.convertedValue, TTCN3ModuleInventory::getInstance().getTypenames(), type_name, res, var);
  name.convertedValue = res;
  bool found = false;
  for (List<Mstring>::iterator vari = variant.begin(); vari; vari = vari->Next) {
    if (vari->Data == "\"untagged\"") {
      found = true;
      break;
    }
  }
  if (!found) {
    addVariant(V_onlyValue, var);
  }
  for (List<RootType*>::iterator dep = nameDepList.begin(); dep; dep = dep->Next) {
    dep->Data->setTypeValue(res);
  }
}

void ComplexType::nameConversion_types(const List<NamespaceType> & ns) {
  attribfields.sort(compareAttributeNameSpaces);
  attribfields.sort(compareAttributeTypes);
  for (List<AttributeType*>::iterator field = attribfields.begin(); field; field = field->Next) {
    field->Data->nameConversion(typeMode, ns);
  }

  for (List<ComplexType*>::iterator field = complexfields.begin(); field; field = field->Next) {
    field->Data->nameConversion_types(ns);
  }

  Mstring prefix, uri, typeValue;

  if (type.convertedValue == "record" ||
    type.convertedValue == "set" ||
    type.convertedValue == "union" ||
    type.convertedValue == "enumerated") {
    return;
  }

  prefix = type.convertedValue.getPrefix(':');
  typeValue = type.convertedValue.getValueWithoutPrefix(':');

  for (List<NamespaceType>::iterator namesp = ns.begin(); namesp; namesp = namesp->Next) {
    if (prefix == namesp->Data.prefix) {
      uri = namesp->Data.uri;
      break;
    }
  }

  QualifiedName in(uri, typeValue); // ns uri + original name

  // Check all known types
  QualifiedNames::iterator origTN = TTCN3ModuleInventory::getInstance().getTypenames().begin();
  for (; origTN; origTN = origTN->Next) {
    if (origTN->Data == in) {
      QualifiedName tmp_name(module->getTargetNamespace(), name.convertedValue);
      if (origTN->Data != tmp_name){
        break;
      }
    }
  }

  if (origTN != NULL) {
    setTypeValue(origTN->Data.name);
  } else {
    Mstring res, var;
    XSDName2TTCN3Name(typeValue, TTCN3ModuleInventory::getInstance().getTypenames(), type_reference_name, res, var, type.no_replace);
    setTypeValue(res);
  }
}

void ComplexType::nameConversion_fields(const List<NamespaceType> & ns) {
  QualifiedNames used_field_names;
  
  for (List<AttributeType*>::iterator field = attribfields.begin(); field; field = field->Next) {
    field->Data->nameConversion_names(used_field_names);
  }

  for (List<ComplexType*>::iterator field = complexfields.begin(); field; field = field->Next) {
    if (field->Data->getMinOccurs() == 0 && field->Data->getMaxOccurs() == 0) {
      continue;
    }
    if (!field->Data->isVisible()) {
      continue;
    }
    
    field->Data->nameConversion_fields(ns);

    Mstring prefix = field->Data->getType().convertedValue.getPrefix(':');
    Mstring typeValue = field->Data->getType().convertedValue.getValueWithoutPrefix(':');

    Mstring res, var;
    var = getModule()->getTargetNamespace();
    XSDName2TTCN3Name(typeValue, TTCN3ModuleInventory::getInstance().getTypenames(), type_reference_name, res, var);

    field->Data->addVariant(V_onlyValue, var);
    var = getModule()->getTargetNamespace();

    if (field->Data->getName().list_extension) {
      field->Data->useNameListProperty();
      XSDName2TTCN3Name(field->Data->getName().convertedValue,
        used_field_names, field_name, res, var);
      field->Data->setNameValue(res);
      bool found_in_variant = false;
      for (List<Mstring>::iterator vari = field->Data->getVariant().begin(); vari; vari = vari->Next) {
        if (vari->Data == Mstring("\"untagged\"")) {
          found_in_variant = true;
          break;
        }
      }
      if (!field->Data->getName().originalValueWoPrefix.empty() &&
        field->Data->getName().originalValueWoPrefix != "sequence" &&
        field->Data->getName().originalValueWoPrefix != "choice" &&
        field->Data->getName().originalValueWoPrefix != "elem" &&
        !found_in_variant) {
        field->Data->addVariant(V_nameAs, field->Data->getName().originalValueWoPrefix);
      }


      if (!found_in_variant) {
        field->Data->addVariant(V_untagged, empty_string, true);
      }
    } else {
      XSDName2TTCN3Name(field->Data->getName().convertedValue,
        used_field_names, field_name, res, var);
      field->Data->setNameValue(res);
      field->Data->addVariant(V_onlyValue, var);
    }

  }
}

void ComplexType::setFieldPaths(Mstring path) {
  if (path.empty()) {
    if (!top) {
      Mstring field_prefix = empty_string;
      if(parent->getMinOccurs() == 0 && parent->getMaxOccurs() == ULLONG_MAX){
        field_prefix = "[-].";
      }
      path = field_prefix + getName().convertedValue;
      actualPath = field_prefix + getName().convertedValue;
    }else {
      actualPath = getName().convertedValue;
    }
  } else if (parent != NULL && (parent->getMinOccurs() != 1 || parent->getMaxOccurs() != 1) &&
             (parent->getName().list_extension || parent->mode == listMode)) {
    path = path + Mstring("[-].") + getName().convertedValue;
    actualPath = path;
  } else {
    path = path + Mstring(".") + getName().convertedValue;
    actualPath = path;
  }

  for (List<ComplexType*>::iterator field = complexfields.begin(); field; field = field->Next) {
    field->Data->setFieldPaths(path);
  }
  for (List<AttributeType*>::iterator attr = attribfields.begin(); attr; attr = attr->Next) {
    attr->Data->setFieldPath(path);
  }
}

void ComplexType::subFinalModification() {  
  //Call SimpleType finalModification
  SimpleType::finalModification();
  
  //Set isOptional field
  isOptional = isOptional || (getMinOccurs() == 0 && getMaxOccurs() == 1);
  
  //
  List<Mstring> enumNames;
  for (List<ComplexType*>::iterator field = complexfields.begin(), nextField; field; field = nextField) {
    nextField = field->Next;
    //Remove invisible fields
    if ((field->Data->getMinOccurs() == 0 && field->Data->getMaxOccurs() == 0) || !field->Data->isVisible()) {
      delete field->Data;
      field->Data = NULL;
      complexfields.remove(field);
    } else {
      //Recursive call
      field->Data->subFinalModification();
      //collect <xsd:all> elements
      if (field->Data->fromAll) {
        enumNames.push_back(field->Data->getName().convertedValue);
      }
    }
  }

  ComplexType * embedField = NULL;
  ComplexType * enumField = NULL;

  //Find the embed and order fields, and remove them
  for (List<ComplexType*>::iterator field = complexfields.begin(), nextField; field; field = nextField) {
    nextField = field->Next;
    if (field->Data->embed) {
      embedField = new ComplexType(*field->Data);
      embedField->parent = this;
      delete field->Data;
      field->Data = NULL;
      complexfields.remove(field);
    } else if (field->Data->enumerated) {
      enumField = new ComplexType(*field->Data);
      enumField->parent = this;
      delete field->Data;
      field->Data = NULL;
      complexfields.remove(field);
    }
  }

  if (enumField != NULL) {
    //Insert the order field in the front
    complexfields.push_front(enumField);
    //Push the field names into the order field
    for (List<Mstring>::iterator field = enumNames.begin(); field; field = field->Next) {
      enumField->enumfields.push_back(field->Data);
    }
  }

  if (embedField != NULL) {
    //Insert the embed field to the front
    complexfields.push_front(embedField);
  }

  if (with_union) {
    unsigned number = 0;
    for (List<ComplexType*>::iterator field = complexfields.begin(); field; field = field->Next) {
      if (field->Data->name.convertedValue.foundAt("alt_") == field->Data->name.convertedValue.c_str()) {
        if (number == 0) {
          field->Data->name.upload(Mstring("alt_"));
        } else {
          expstring_t new_name = mprintf("alt_%d", number);
          field->Data->name.upload(Mstring(new_name));
          Free(new_name);
        }
        number++;
      }
    }
  }

  AttributeType * anyAttr = NULL;
  for (List<AttributeType*>::iterator field = attribfields.begin(), nextField; field; field = nextField) {
    nextField = field->Next;
    field->Data->applyUseAttribute();
    //Find anyattribute, and remove it
    if (field->Data->isAnyAttribute()) {
      anyAttr = new AttributeType(*field->Data);
      setParent(this, anyAttr);
      delete field->Data;
      field->Data = NULL;
      attribfields.remove(field);
    } else if (field->Data->getUseVal() == prohibited || !field->Data->isVisible()) {
      //Not visible attribute removed
      delete field->Data;
      field->Data = NULL;
      attribfields.remove(field);
    } else {
      field->Data->SimpleType::finalModification();
    }
  }
  
  //Push anyattribute to the front
  if (anyAttr != NULL) {
    anyAttr->applyNamespaceAttribute(V_anyAttributes);
    attribfields.push_back(anyAttr);
  }

  //Substitution group ordering
  if(subsGroup == this || typeSubsGroup == this){ //We are a generated substitution group
    //Substitution group never empty
    ComplexType * front = complexfields.front();
    List<ComplexType*>::iterator it = complexfields.begin();
    complexfields.remove(it);
    complexfields.sort(compareComplexTypeNameSpaces);
    complexfields.sort(compareTypes);
    complexfields.push_front(front);
  }
}

void ComplexType::finalModification() {
  subFinalModification();
  setFieldPaths(empty_string);
}

void ComplexType::finalModification2() {
  subFinalModification2();
  List<Mstring> container;
  collectVariants(container);
  variant.clear();
  variant = container;
}

void ComplexType::subFinalModification2() {
  SimpleType::finalModification2();
  for (List<ComplexType*>::iterator field = complexfields.begin(); field; field = field->Next) {
    //Recursive call
    field->Data->subFinalModification2();
  }
  for (List<AttributeType*>::iterator field = attribfields.begin(); field; field = field->Next) {
    field->Data->SimpleType::finalModification2();
  }
}

void ComplexType::printToFile(FILE * file) {
  printToFile(file, 0, false);
}

void ComplexType::printToFile(FILE * file, const unsigned level, const bool is_union) {
  if (!isVisible()) {
    return;
  }
  printComment(file, level);
  if (top) {
    fprintf(file, "type ");
    if(mode == listMode){
      printMinOccursMaxOccurs(file, is_union);
      fprintf(file, "%s", type.convertedValue.c_str());
    }else {
      fprintf(file, "%s %s", type.convertedValue.c_str(), name.convertedValue.c_str());
    }
    if(type.convertedValue == "record" || type.convertedValue == "union"){
      fprintf(file, "\n{\n");
      if (attribfields.empty() && complexfields.empty()) {
        fprintf(file, "\n");
      }
    } 

    for (List<ComplexType*>::iterator c = complexfields.begin(), nextField; c; c = nextField) {
      nextField = c->Next;
      if (c->Data->embed || c->Data->enumerated) {
        c->Data->printToFile(file, level + 1, is_union);
        if (c->Next != NULL || !attribfields.empty()) {
          fprintf(file, ",\n");
        } else {
          fprintf(file, "\n");
        }
        delete c->Data;
        c->Data = NULL;
        complexfields.remove(c);
      }
    }

    for (List<AttributeType*>::iterator f = attribfields.begin(); f; f = f->Next) {
      f->Data->printToFile(file, level + 1);
      if (f->Next != NULL || !complexfields.empty()) {
        fprintf(file, ",\n");
      } else {
        fprintf(file, "\n");
      }
    }

    for (List<ComplexType*>::iterator c = complexfields.begin(); c; c = c->Next) {
      c->Data->printToFile(file, level + 1, is_union);
      if (c->Next != NULL) {
        fprintf(file, ",\n");
      } else {
        fprintf(file, "\n");
      }
    }
  } else {
    const bool field_is_record = getType().convertedValue == Mstring("record");
    const bool field_is_union = getType().convertedValue == "union";
    if (complexfields.empty() && attribfields.empty() && (field_is_record || field_is_union)) {
      if (field_is_record) {
        indent(file, level);
        printMinOccursMaxOccurs(file, is_union);
        fprintf(file, "%s {\n", getType().convertedValue.c_str());
        indent(file, level);
        fprintf(file, "} %s", getName().convertedValue.c_str());
        if (isOptional) {
          fprintf(file, " optional");
        }
      } else if (field_is_union) {
        indent(file, level);
        printMinOccursMaxOccurs(file, is_union);
        fprintf(file, "%s {\n", getType().convertedValue.c_str());
        indent(file, level + 1);
        fprintf(file, "record length(0 .. 1) of enumerated { NULL_ } choice\n");
        indent(file, level);
        fprintf(file, "} %s", getName().convertedValue.c_str());
        if (isOptional) {
          fprintf(file, " optional");
        }
      }
    } else {
      indent(file, level);
      if (getEnumeration().modified) {
        if (isFloatType(getBuiltInBase())) {
          fprintf(file, "%s (", type.convertedValue.c_str());
          getEnumeration().sortFacets();
          getEnumeration().printToFile(file);
          fprintf(file, ")");
        } else {
          printMinOccursMaxOccurs(file, with_union);
          fprintf(file, "enumerated {\n");
          //getEnumeration().sortFacets();
          getEnumeration().printToFile(file, level);
          fprintf(file, "\n");
          indent(file, level);
          fprintf(file, "} ");
        }
      } else {
        int multiplicity = multi(module, getReference(), this);
        if ((multiplicity > 1) && getReference().get_ref()) {
          fprintf(file, "%s.", getReference().get_ref()->getModule()->getModulename().c_str());
        }
        if (field_is_record || field_is_union || listPrint) {
          unsigned long long int tempMin = getMinOccurs();
          unsigned long long int tempMax = getMaxOccurs();
          if (listPrint) {
            setMinOccurs(listMinOccurs);
            setMaxOccurs(listMaxOccurs);
          }
          printMinOccursMaxOccurs(file, with_union, !first_child || parent->getXsdtype() != n_choice);
          if (listPrint) {
            setMinOccurs(tempMin);
            setMaxOccurs(tempMax);
          }
          if (listPrint && complexfields.size() == 0) {
            printMinOccursMaxOccurs(file, false);
            fprintf(file, "%s ",getType().convertedValue.c_str());
          } else {
            fprintf(file, "%s {\n", getType().convertedValue.c_str());
            for (List<AttributeType*>::iterator f = attribfields.begin(); f; f = f->Next) {
              f->Data->printToFile(file, level + 1);
              if (f->Next != NULL || !complexfields.empty()) {
                fprintf(file, ",\n");
              } else {
                fprintf(file, "\n");
              }
            }

            for (List<ComplexType*>::iterator c = complexfields.begin(); c; c = c->Next) {
              c->Data->printToFile(file, level + 1, is_union);
              if (c->Next != NULL) {
                fprintf(file, ",\n");
              } else {
                fprintf(file, "\n");
              }
            }
          }
        } else {
          printMinOccursMaxOccurs(file, with_union, !first_child);
          fprintf(file, "%s ", getType().convertedValue.c_str());
          if (getName().convertedValue == Mstring("order") && getType().convertedValue == Mstring("enumerated")) {
            fprintf(file, "{\n");
            for (List<Mstring>::iterator e = enumfields.begin(); e; e = e->Next) {
              indent(file, level + 1);
              fprintf(file, "%s", e->Data.c_str());
              if (e->Next != NULL) {
                fprintf(file, ",\n");
              } else {
                fprintf(file, "\n");
              }
            }
            indent(file, level);
            fprintf(file, "} ");
          }
        }
      }
      if ((field_is_record || field_is_union) && !listPrint) {
        indent(file, level);
        fprintf(file, "} ");
      }

      fprintf(file, "%s", getName().convertedValue.c_str());
      getPattern().printToFile(file);
      getValue().printToFile(file);
      getLength().printToFile(file);
      if (!with_union && isOptional) {
        fprintf(file, " optional");
      }
    }
  }

  if (top) {
    if(type.convertedValue == "record" || type.convertedValue == "union"){
      fprintf(file, "}");
    }
    if(mode == listMode){
      fprintf(file, " %s", name.convertedValue.c_str());
    }
    printVariant(file);
    fprintf(file, ";\n\n\n");
  }
}

void ComplexType::collectVariants(List<Mstring>& container) {

  if (e_flag_used || !isVisible()) {
    return;
  }

  if (top) {
    bool useUnionVariantWhenMainTypeIsRecordOf = false;
    for (List<Mstring>::iterator var = variant.end(); var; var = var->Prev) {
      if ((getMinOccurs() != 1 || getMaxOccurs() != 1) && (var->Data == "\"useUnion\"")) { // main type is a record of
        useUnionVariantWhenMainTypeIsRecordOf = true; // TR HL15893
      } else {
        container.push_back(Mstring("variant ") + Mstring(var->Data.c_str()) + Mstring(";\n"));
      }
    }
    if (useUnionVariantWhenMainTypeIsRecordOf) {
      container.push_back(Mstring("variant ([-]) \"useUnion\";\n"));
    }
    for (List<Mstring>::iterator var = hidden_variant.end(); var; var = var->Prev) {
      container.push_back(Mstring("//variant ") + Mstring(var->Data.c_str()) + Mstring(";\n"));
    }
  }

  //Collect variants of attributes
  for (List<AttributeType*>::iterator field = attribfields.begin(); field; field = field->Next) {
    field->Data->collectVariants(container);
  }

  for (List<ComplexType*>::iterator field = complexfields.begin(); field; field = field->Next) {

    if (!field->Data->isVisible()) {
      continue;
    }

    if (field->Data->getVariant().empty() && field->Data->getHiddenVariant().empty() &&
        field->Data->complexfields.empty() && field->Data->attribfields.empty() &&
        field->Data->enumeration.variants.empty()) {
      continue;
    }

    bool already_used = false;

    for (List<Mstring>::iterator var2 = field->Data->getVariant().end(); var2; var2 = var2->Prev) {
      if (var2->Data == "\"untagged\"" && !already_used) {
        container.push_back(Mstring("variant (") + field->Data->actualPath + Mstring(") ") + Mstring(var2->Data.c_str()) + Mstring(";\n"));
        already_used = true;
      } else {
        if ((field->Data->getMinOccurs() != 1 || field->Data->getMaxOccurs() != 1) &&
          (field->Data->getName().list_extension || var2->Data == "\"useUnion\"")) {
          container.push_back(Mstring("variant (") + field->Data->actualPath + Mstring("[-]) ") + Mstring(var2->Data.c_str()) + Mstring(";\n"));
        } else if (var2->Data != "\"untagged\"") {
          container.push_back(Mstring("variant (") + field->Data->actualPath + Mstring(") ") + Mstring(var2->Data.c_str()) + Mstring(";\n"));
        }
      }
    }
    for (List<Mstring>::iterator hidden_var = field->Data->getHiddenVariant().end();
      hidden_var; hidden_var = hidden_var->Prev) {
      if ((field->Data->getMinOccurs() != 1 || field->Data->getMaxOccurs() != 1) &&
        field->Data->getName().list_extension) {
        container.push_back(Mstring("//variant (") + field->Data->actualPath + Mstring("[-]) ") + Mstring(hidden_var->Data.c_str()) + Mstring(";\n"));
      } else {
        container.push_back(Mstring("//variant (") + field->Data->actualPath + Mstring(") ") + Mstring(hidden_var->Data.c_str()) + Mstring(";\n"));
      }
    }

    if(field->Data->enumeration.modified){
      Mstring path = empty_string;
      if(field->Data->getMinOccurs() != 1 && field->Data->getMaxOccurs() != 1){
        path = field->Data->actualPath + Mstring("[-]");
      }else {
        path = field->Data->actualPath;
      }
      for(List<Mstring>::iterator var = field->Data->enumeration.variants.end(); var; var = var->Prev){
        if(var->Data.empty()) continue;
        container.push_back("variant (" + path + ") " + var->Data + ";\n");
      }
    }
    //Recursive call
    field->Data->collectVariants(container);
  }
}

void ComplexType::printVariant(FILE * file) {
  if (e_flag_used) {
    return;
  }

  bool foundAtLeastOneVariant = false;
  bool foundAtLeastOneHiddenVariant = false;

  if (!variant.empty()) {
    for (List<Mstring>::iterator var = variant.begin(); var; var = var->Next) {
      if (foundAtLeastOneVariant && foundAtLeastOneHiddenVariant) {
        break;
      }
      if (var->Data[0] != '/') {
        foundAtLeastOneVariant = true;
      } else {
        foundAtLeastOneHiddenVariant = true;
      }
    }
  }

  if (!foundAtLeastOneVariant && !foundAtLeastOneHiddenVariant) {
    return;
  }

  if (!foundAtLeastOneVariant) {
    //No other variants, only commented, so the 'with' must be commented also.
    fprintf(file, ";\n//with {\n");
  } else {
    fprintf(file, "\nwith {\n");
  }

  for (List<Mstring>::iterator var = variant.begin(); var; var = var->Next) {
    fprintf(file, "  %s", var->Data.c_str());
  }

  if (!foundAtLeastOneVariant) {
    fprintf(file, "//");
  }
  fprintf(file, "}");
}

void ComplexType::dump(unsigned int depth) const {
  fprintf(stderr, "%*s %sComplexType at %p | Top:%s\n", depth * 2, "", isVisible() ? "" : "(hidden)", (const void*) this, top ? "true" : "false");
  if (parent != NULL) {
    fprintf(stderr, "%*s parent: %p | parent xsdtype: %i | my xsdtype: %i\n", depth * 2, "", (const void*) parent, parent->getXsdtype(), xsdtype);
  } else {
    fprintf(stderr, "%*s parent: %p | parent xsdtype: %s | my xsdtype: %i\n", depth * 2, "", "NULL", "NULL", xsdtype);
  }
  fprintf(stderr, "%*s name='%s' -> '%s', type='%s' %d complexfields | %s | %s | %s\n", depth * 2, "",
    name.originalValueWoPrefix.c_str(), name.convertedValue.c_str(), type.convertedValue.c_str(), (int) complexfields.size(),
    outside_reference.empty() ? "" : outside_reference.get_val().c_str(), mode == restrictionMode ? "restriction" : "",
    mode == extensionMode ? "extension" : "");
  for (List<ComplexType*>::iterator field = complexfields.begin(); field; field = field->Next) {
    field->Data->dump(depth + 1);
  }
  fprintf(stderr, "%*s %d attribields\n", depth * 2, "", (int) attribfields.size());
  for (List<AttributeType*>::iterator field = attribfields.begin(); field; field = field->Next) {
    field->Data->dump(depth + 1);
  }
  fprintf(stderr, "%*s %d enumfields\n", depth * 2, "", (int) enumfields.size());
  for (List<Mstring>::iterator field = enumfields.begin(); field; field = field->Next) {
    fprintf(stderr, "%*s enum: %s\n", depth * 2 + depth, "", field->Data.c_str());
  }
  fprintf(stderr, "%*s (%llu .. %llu) | Optional:%s | List:%s\n", (depth + 1) * 2, "", getMinOccurs(), getMaxOccurs(), isOptional ? "true" : "false", name.list_extension ? "true" : "false");
  fprintf(stderr, "%*s %d variants: ", (depth + 1) * 2, "", (int) variant.size());
  for (List<Mstring>::iterator var = variant.begin(); var; var = var->Next) {
    fprintf(stderr, "%s, ", var->Data.c_str());
  }
  fprintf(stderr, "%*s pattern:%s | length:%i \n ", (depth + 1) * 2, "", this->pattern.facet.c_str(), (int) (this->length.facet_maxLength));
  fprintf(stderr, "%*s enum: %i \n", (depth + 1)*2, "", (int) this->enumeration.facets.size());
  fprintf(stderr, "\n");
}

void ComplexType::setMinMaxOccurs(const unsigned long long min, const unsigned long long max, const bool generate_list_postfix) {

  if (min != 1 || max != 1) {
    if (xsdtype == n_choice) {
      setMinOccurs(min);
      setMaxOccurs(max);
      addVariant(V_untagged);
      first_child = false;
    } else if (xsdtype == n_sequence) {
      ComplexType * rec = new ComplexType(this);
      rec->type.upload(Mstring("record"), false);
      rec->name.upload(Mstring("sequence"));
      rec->setXsdtype(n_sequence);
      rec->addVariant(V_untagged);
      rec->addVariant(V_untagged);
      rec->setMinOccurs(min);
      rec->setMaxOccurs(max);
      complexfields.push_back(rec);
      actfield = rec;
      if ((rec->getMinOccurs() == 0 && rec->getMaxOccurs() > 1) || rec->getMinOccurs() > 0) {
        rec->name.list_extension = true;
      }
    } else {
      setMinOccurs(min);
      setMaxOccurs(max);
      if ((getMinOccurs() == 0 && getMaxOccurs() > 1) || getMinOccurs() > 0) {
        if (generate_list_postfix) {
          name.list_extension = true;
        }
      }
      if (parent != NULL && parent->getXsdtype() == n_choice) {
        name.list_extension = true;
        if (parent->first_child == false && getMinOccurs() == 0) {
          parent->first_child = true;
          with_union = true;
          first_child = false;
        } else {
          with_union = true;
          first_child = true;
        }       
      }
    }
  }

  if (getMaxOccurs() > 1 && generate_list_postfix) {
    name.list_extension = true;
  }
}

void ComplexType::applyNamespaceAttribute(VariantMode varLabel, const Mstring& ns_list) {
  List<Mstring> namespaces;
  if (!ns_list.empty()) {
    expstring_t valueToSplitIntoTokens = mcopystr(ns_list.c_str());
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
    static char xxany[] = "##any", xxother[] = "##other", xxlocal[] = "##local",
      xxtargetNamespace[] = "##targetNamespace";
    if (!first) any_ns += ',';

    if (ns->Data == xxany) {
    }// this must be the only element, nothing to add
    else if (ns->Data == xxother) { // this must be the only element
      any_ns += " except unqualified";
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

    first = false;
  }

  addVariant(varLabel, any_ns, true);
}

void ComplexType::addComment(const Mstring& text) {
  if (this == actfield) {
    if (lastType == n_attribute) { // ez nem teljesen jo, stb, tobb lehetoseg, es rossy sorrend
      if (!attribfields.empty()) {
        attribfields.back()->addComment(text);
      }
    } else {
      if (actfield->getName().convertedValue == Mstring("base") && parent != NULL) {
        parent->getComment().push_back(Mstring("/* " + text + " */\n"));
      } else {
        comment.push_back(Mstring("/* " + text + " */\n"));
      }
    }
  } else {
    actfield->addComment(text);
    return;
  }
}

//Attribute extension logic when extending complextypes
void ComplexType::applyAttributeExtension(ComplexType * found_CT, AttributeType * anyAttrib /* = NULL */) {
  for (List<AttributeType*>::iterator attr = found_CT->attribfields.begin(); attr; attr = attr->Next) {
    bool l = false;
    if (anyAttrib != NULL && attr->Data->isAnyAttribute()) {
      anyAttrib->addNameSpaceAttribute(attr->Data->getNameSpaceAttribute());
      l = true;
    } else {
      for (List<AttributeType*>::iterator attr2 = attribfields.begin(); attr2; attr2 = attr2->Next) {
        if (attr->Data->getName().convertedValue == attr2->Data->getName().convertedValue &&
          attr->Data->getType().convertedValue == attr2->Data->getType().convertedValue) {
          if (attr->Data->getUseVal() == optional) {
            attr2->Data->setUseVal(optional);
          }
          l = true;
          break;
        }
      }
    }
    if (!l) {
      AttributeType * newAttrib = new AttributeType(*attr->Data);
      attribfields.push_back(newAttrib);
      setParent(this, newAttrib);
    }
  }
}

//Attribute restriction logic when restricting complextypes
bool ComplexType::applyAttributeRestriction(ComplexType * found_CT) {
  bool isModifiedAttr = false;
  // when the element is nillable the attributes are in the parent
  ComplexType * me = parent != NULL && nillable ? parent : this;
  for (List<AttributeType*>::iterator attr = me->attribfields.begin(), nextAttr; attr; attr = nextAttr) {
    nextAttr = attr->Next;
    bool l = false;
    for (List<AttributeType*>::iterator attr2 = found_CT->attribfields.begin(); attr2; attr2 = attr2->Next) {
      if (attr->Data->getName().convertedValue == attr2->Data->getName().convertedValue &&
        attr->Data->getType().convertedValue == attr2->Data->getType().convertedValue) {
        l = true;
        break;
      }
    }
    if (!l) {
      delete attr->Data;
      attr->Data = NULL;
      me->attribfields.remove(attr);
    }
  }
  size_t size = found_CT->attribfields.size();
  size_t size2 = me->attribfields.size();
  size_t i = 0;
  List<AttributeType*>::iterator attr = found_CT->attribfields.begin();
  for (; i < size; attr = attr->Next, i = i + 1) {
    bool l = false;
    size_t j = 0;
    List<AttributeType*>::iterator attr2 = me->attribfields.begin();
    for (; j < size2; attr2 = attr2->Next, j = j + 1) {
      if (attr->Data->getName().convertedValue == attr2->Data->getName().convertedValue &&
        attr->Data->getType().convertedValue == attr2->Data->getType().convertedValue && !attr2->Data->getUsed()) {
        l = true;
        if(attr->Data->getUseVal() != attr2->Data->getUseVal()){
          isModifiedAttr = true;
        }
        attr2->Data->setUsed(true);
        break;
      }
    }
    if (!l) {
      AttributeType * newAttrib = new AttributeType(*attr->Data);
      me->attribfields.push_back(newAttrib);
      setParent(me, newAttrib);
    }
  }
  return isModifiedAttr;
}

void ComplexType::addNameSpaceAsVariant(RootType * root, RootType * other) {
  if (other->getModule()->getTargetNamespace() != root->getModule()->getTargetNamespace() &&
    other->getModule()->getTargetNamespace() != "NoTargetNamespace") {
    root->addVariant(V_namespaceAs, other->getModule()->getTargetNamespace());
  }
}

void ComplexType::resolveAttribute(AttributeType* attr) {
  if (attr->getXsdtype() == n_attribute && !attr->getReference().empty()) {
    SimpleType * st = (SimpleType*) TTCN3ModuleInventory::getInstance().lookup(attr, want_BOTH);
    if (st != NULL) {
      if (attr->isFromRef()) {
        addNameSpaceAsVariant(attr, st);
        attr->setTypeOfField(st->getName().convertedValue);
        attr->setNameOfField(st->getName().originalValueWoPrefix);
        attr->setOrigModule(st->getModule());
        st->addToNameDepList(attr);
        if (attr->getConstantDefaultForEmpty() != NULL) {
          st->addToNameDepList((RootType*)attr->getConstantDefaultForEmpty());
        }
      } else {
        attr->setTypeOfField(st->getName().convertedValue);
        if (st->getType().convertedValue == "record" || st->getType().convertedValue == "union"
            || st->getXsdtype() == n_NOTSET) // It really is a simpleType
          {
            st->addToNameDepList(attr);
            if (attr->getConstantDefaultForEmpty() != NULL) {
              st->addToNameDepList((RootType*)attr->getConstantDefaultForEmpty());
            }
          }
      }
      attr->getReference().set_resolved(st);
    } else {
      printError(module->getSchemaname(), name.convertedValue,
        "Reference for a non-defined type: " + attr->getReference().repr());
      TTCN3ModuleInventory::getInstance().incrNumErrors();
    }
  }
}

void ComplexType::resolveAttributeGroup(SimpleType * st) {
  if (xsdtype == n_attributeGroup && !outside_reference.empty()) {
    ComplexType * ct = (ComplexType*) st;
    if(ct->resolved == No){
      ct->referenceResolving();
    }
    outside_reference.set_resolved(ct);
    setInvisible();
    bool addNameSpaceas = false;
    if (ct->getModule()->getTargetNamespace() != module->getTargetNamespace() &&
      ct->getModule()->getTargetNamespace() != "NoTargetNamespace") {
      addNameSpaceas = true;
    }
    ComplexType * par;
    if(parent->nillable && parent->parent != NULL){
      par = parent->parent;
    }else {
      par = parent;
    }
    List<AttributeType*>::iterator anyAttrib = par->attribfields.begin();
    for (; anyAttrib; anyAttrib = anyAttrib->Next) {
      if (anyAttrib->Data->isAnyAttribute()) {
        break;
      }
    }
    for (List<AttributeType*>::iterator attr = ct->attribfields.begin(); attr; attr = attr->Next) {
      if (anyAttrib != NULL && attr->Data->isAnyAttribute()) {
        anyAttrib->Data->addNameSpaceAttribute(attr->Data->getNameSpaceAttribute());
      } else {
        AttributeType * attrib = new AttributeType(*attr->Data);
        attr->Data->setOrigModule(ct->getModule());
        if (addNameSpaceas) {
          attrib->addVariant(V_namespaceAs, ct->getModule()->getTargetNamespace());
        }
        //Nillable attribute placement is hard...
        if (parent->nillable && parent->parent != NULL) {
          parent->parent->attribfields.push_back(attrib);
          attrib->parent = parent->parent;
          setParent(parent->parent, attrib);
        } else if (parent->nillable && !parent->complexfields.empty()) {
          parent->complexfields.back()->attribfields.push_back(attrib);
          attrib->parent = parent->complexfields.back();
        } else if (parent->parent != NULL && (parent->parent->mode == extensionMode || parent->parent->mode == restrictionMode)) {
          parent->parent->attribfields.push_back(attrib);
          setParent(parent->parent, attrib);
        } else {
          parent->attribfields.push_back(attrib);
          setParent(parent, attrib);
        }
      }
    }
  }
}

void ComplexType::resolveGroup(SimpleType *st) {
  if (xsdtype == n_group && !outside_reference.empty()) {
    ComplexType * ct = (ComplexType*) st;
    outside_reference.set_resolved(ct);
    setInvisible();
    if(ct->resolved == No){
      ct->referenceResolving();
    }
    //Decide if namespaceas variant needs to be added
    bool addNameSpaceas = false;
    if (ct->getModule()->getTargetNamespace() != module->getTargetNamespace() &&
      ct->getModule()->getTargetNamespace() != "NoTargetNamespace") {
      addNameSpaceas = true;
    }
    if (ct->getXsdtype() == n_sequence && getMinOccurs() == 1 && getMaxOccurs() == 1 && (parent->getXsdtype() == n_complexType || parent->getXsdtype() == n_sequence)) {
      for (List<ComplexType*>::iterator c = ct->complexfields.begin(); c; c = c->Next) {
        ComplexType * newField = new ComplexType(*c->Data);
        parent->complexfields.push_back(newField);
        setParent(parent, newField);
        parent->complexfields.back()->setModule(getModule());
        if (addNameSpaceas) {
          parent->complexfields.back()->addVariant(V_namespaceAs, ct->getModule()->getTargetNamespace());
        }
      }
    } else if (ct->getXsdtype() == n_all) {
      //If the parent optional, then every field is optional
      for (List<ComplexType*>::iterator c = ct->complexfields.begin(); c; c = c->Next) {
        ComplexType* f = new ComplexType(*c->Data);
        if (getMinOccurs() == 0 && !f->enumerated) {
          f->isOptional = true;
        }
        ((ComplexType*) parent)->complexfields.push_back(f);
        setParent(parent, f);
        f->setModule(getModule());
        if (addNameSpaceas) {
          f->addVariant(V_namespaceAs, ct->getModule()->getTargetNamespace());
        }
      }
      parent->addVariant(V_useOrder);
    } else {
      if (name.list_extension) {
        addVariant(V_untagged);
      }
      type.upload(ct->getName().convertedValue);
      name.upload(ct->getName().convertedValue);
      ct->addToNameDepList(this);
      nameDep = ct;
      visible = true;
      if (addNameSpaceas) {
        addVariant(V_namespaceAs, ct->getModule()->getTargetNamespace());
      }
    }
  }
}

void ComplexType::resolveElement(SimpleType *st) {
  if (xsdtype == n_element && !outside_reference.empty()) {
    outside_reference.set_resolved(st);
    type.upload(st->getModule()->getTargetNamespaceConnector() + Mstring(":") + st->getName().convertedValue);
    if (name.originalValueWoPrefix.empty()) {
      name.upload(st->getName().convertedValue);
    }
    if (fromRef) {
      addNameSpaceAsVariant(this, st);
    }

    collectElementTypes(st, NULL);

    //Namedep is added to the substitutions, if any
    if(st->getSubstitution() != NULL){
      st->getSubstitution()->addToNameDepList(this);
      nameDep = st->getSubstitution();
      if (defaultForEmptyConstant != NULL) {
        st->getSubstitution()->addToNameDepList((RootType*)defaultForEmptyConstant);
      }
    }if(st->getTypeSubstitution() != NULL){
      st->getTypeSubstitution()->addToNameDepList(this);
      nameDep = st->getTypeSubstitution();
      if (defaultForEmptyConstant != NULL) {
        st->getTypeSubstitution()->addToNameDepList((RootType*)defaultForEmptyConstant);
      }
    }else {
      st->addToNameDepList(this);
      nameDep = st;
      if (defaultForEmptyConstant != NULL) {
        st->addToNameDepList((RootType*)defaultForEmptyConstant);
      }
    }
  }
}

void ComplexType::resolveSimpleTypeExtension() {
  if (mode == extensionMode && cmode == CT_simpletype_mode && basefield != NULL) {
    SimpleType * st = (SimpleType*) TTCN3ModuleInventory::getInstance().lookup(basefield, want_BOTH);
    if (st != NULL) {
      if (st->getXsdtype() != n_NOTSET && ((ComplexType*) st)->basefield != NULL) { // if the xsdtype != simpletype
        ComplexType * ct = (ComplexType*) st;
        if (ct->resolved == No) {
          ct->referenceResolving();
        }
        // If an alias
        if (attribfields.empty()) {
          for (List<ComplexType*>::iterator field = complexfields.begin(); field; field = field->Next) {
            field->Data->setInvisible();
          }

          for (List<AttributeType*>::iterator field = attribfields.begin(); field; field = field->Next) {
            field->Data->setInvisible();
          }
          
          basefield->setInvisible();
          type.upload(st->getName().originalValueWoPrefix);
          ct->addToNameDepList(this);
          nameDep = ct;
          addNameSpaceAsVariant(this, st);
          alias = ct;
        } else {
          
          while (ct != NULL && ct->getAlias() != NULL) {
            ct = (ComplexType*)ct->getAlias();
          }
          
          basefield->outside_reference.set_resolved(ct);
          ct->basefield->addToNameDepList(basefield);
          basefield->nameDep = ct->basefield;
          basefield->mode = extensionMode;
          basefield->applyReference(*ct->basefield, true);
          addNameSpaceAsVariant(basefield, ct->basefield);
          applyAttributeExtension(ct);
        }
      } else {
        if (!st->getReference().empty() && !st->getReference().is_resolved()) {
          st->referenceResolving();
        }
        
        bool hasRestrOrExt = basefield->hasRestrictionOrExtension();
        // If an alias
        if(!hasRestrOrExt && attribfields.empty()){
          // Set the fields and attributes invisible
          for (List<ComplexType*>::iterator field = complexfields.begin(); field; field = field->Next) {
            field->Data->setInvisible();
          }

          for (List<AttributeType*>::iterator field = attribfields.begin(); field; field = field->Next) {
            field->Data->setInvisible();
          }
          basefield->setInvisible();
          type.upload(st->getName().originalValueWoPrefix);
          st->addToNameDepList(this);
          nameDep = st;
          addNameSpaceAsVariant(this, st);
          alias = st;
        } else {
          while(st != NULL && st->getAlias() != NULL) {
            st = st->getAlias();
          }
          basefield->builtInBase = st->getBuiltInBase();
          addNameSpaceAsVariant(basefield, st);
          const Mstring old_type = basefield->getType().originalValueWoPrefix;
          basefield->applyReference(*st);
          // If st has enumeration then the type is restored to the original value
          // because enumerations cannot be extended here and this way we just
          // create an alias.
          if (st->getEnumeration().modified) {
            basefield->setTypeValue(old_type);
            basefield->getEnumeration().modified = false;
          }
        }
      }
    } else if(!isBuiltInType(basefield->getType().convertedValue)){
         printError(module->getSchemaname(), name.convertedValue,
          "Reference for a non-defined type: " + basefield->getReference().repr());
       TTCN3ModuleInventory::getInstance().incrNumErrors();
       return;
    }

  }
}

void ComplexType::resolveSimpleTypeRestriction() {
  if (mode == restrictionMode && cmode == CT_simpletype_mode && basefield != NULL && !basefield->outside_reference.empty()) {
    SimpleType * st = (SimpleType*) TTCN3ModuleInventory::getInstance().lookup(basefield, want_BOTH);
    if (st == NULL) {
      printError(module->getSchemaname(), name.convertedValue,
        "Reference for a non-defined type: " + basefield->getReference().repr());
      TTCN3ModuleInventory::getInstance().incrNumErrors();
      return;
    }
    bool hasRestrOrExt = basefield->hasRestrictionOrExtension();
    // If an alias
    if(!hasRestrOrExt && attribfields.empty()){
      type.upload(st->getName().convertedValue);
      st->addToNameDepList(this);
      nameDep = st;
      alias = st;
      basefield->setInvisible();
      for (List<ComplexType*>::iterator field = complexfields.begin(); field; field = field->Next) {
        field->Data->setInvisible();
      }

      for (List<AttributeType*>::iterator field = attribfields.begin(); field; field = field->Next) {
        field->Data->setInvisible();
      }
      addNameSpaceAsVariant(this, st);
      return;
    }
    
    while (st != NULL && st->getAlias() != NULL) {
      st = st->getAlias();
    }
    basefield->outside_reference.set_resolved(st);
    if (st->getXsdtype() != n_NOTSET) {
      ComplexType * ct = (ComplexType*) st;
      if (ct->resolved == No) {
        ct->referenceResolving();
      }
      applyAttributeRestriction(ct);
      basefield->mode = restrictionMode;
      if (ct->cmode == CT_complextype_mode) {
        applyReference(*ct, true);
        type.upload(ct->getName().convertedValue);
        basefield->setInvisible();
      } else if (ct->basefield != NULL) {
        basefield->applyReference(*ct->basefield);
        addNameSpaceAsVariant(basefield, ct->basefield);
      } else if (ct->basefield == NULL) {
        basefield->applyReference(*ct);
        addNameSpaceAsVariant(basefield, ct);
      }
    } else {
      if (!st->getReference().empty() && !st->getReference().is_resolved()) {
        st->referenceResolving();
      }
      if(xsdtype == n_simpleContent){
        basefield->applyReference(*st, true);
        addNameSpaceAsVariant(basefield, st);
        basefield->mode = restrictionMode;
      }else if(xsdtype == n_simpleType){
        basefield->setInvisible();
        applyReference(*basefield, true);
        applyReference(*st, true);
        addNameSpaceAsVariant(this, st);
        basefield->mode = restrictionMode;
        }
    }
  } else if (mode == restrictionMode && cmode == CT_simpletype_mode && basefield != NULL) {
    ComplexType * ct = (ComplexType*) TTCN3ModuleInventory::getInstance().lookup(basefield, want_CT);
    if (ct == NULL && !isBuiltInType(basefield->getType().convertedValue)) {
      printError(module->getSchemaname(), name.convertedValue,
        "Reference for a non-defined type: " + basefield->getReference().repr());
      TTCN3ModuleInventory::getInstance().incrNumErrors();
      return;
    }
    
    // Alias not possible here, because basefield->outside_reference is empty
    basefield->outside_reference.set_resolved(ct);
    if (ct != NULL) {
      if (ct->resolved == No) {
        ct->referenceResolving();
      }
      for (List<AttributeType*>::iterator f = ct->attribfields.begin(); f; f = f->Next) {
        AttributeType * attr = new AttributeType(*f->Data);
        attribfields.push_back(attr);
        setParent(this, attr);
      }
      addNameSpaceAsVariant(this, ct);
    }
    if(!basefield->parent->top){
      // This is the case of restriction -> list -> simpletype -> restriction
      // we have to apply the reference to the parent's parent.
      if(basefield->parent->parent != NULL && !basefield->parent->isVisible()) {
        basefield->parent->parent->applyReference(*basefield, true);
      } else {
        applyReference(*basefield, true);
      }
      basefield->setInvisible();
    }
  }
}

void ComplexType::resolveComplexTypeExtension() {
  if (mode == extensionMode && cmode == CT_complextype_mode && !outside_reference.empty()) {
    ComplexType * ct = (ComplexType*) TTCN3ModuleInventory::getInstance().lookup(this, want_CT);
    if (ct == NULL) {
      printError(module->getSchemaname(), name.convertedValue,
        "Reference for a non-defined type: " + getReference().repr());
      TTCN3ModuleInventory::getInstance().incrNumErrors();
      return;
    }
    if(ct->getXsdtype() != n_NOTSET){
      outside_reference.set_resolved(ct);
      if (ct->resolved == No) {
        ct->referenceResolving();
      }
      // it is an alias
      if (complexfields.empty() && attribfields.empty()) {
        type.upload(ct->getName().convertedValue);
        
        for (List<ComplexType*>::iterator field = complexfields.begin(); field; field = field->Next) {
          field->Data->setInvisible();
        }
        
        for (List<AttributeType*>::iterator field = attribfields.begin(); field; field = field->Next) {
          field->Data->setInvisible();
        }
        ct->addToNameDepList(this);
        nameDep = ct;
        alias = ct;
        return;
      }
      while (ct != NULL && ct->getAlias() != NULL) {
        ct = (ComplexType*)ct->getAlias();
      }
      List<AttributeType*>::iterator anyAttr = attribfields.begin();
      for (; anyAttr; anyAttr = anyAttr->Next) {
        if (anyAttr->Data->isAnyAttribute()) {
          break;
        }
      }

      if (anyAttr != NULL) {
        applyAttributeExtension(ct, anyAttr->Data);
      } else {
        applyAttributeExtension(ct);
      }

      if (ct->getName().convertedValue == outside_reference.get_val() && ct->getModule()->getTargetNamespace() == outside_reference.get_uri()) {
        //Self recursion
        outside_reference.set_resolved(ct);
        for (List<ComplexType*>::iterator f = ct->complexfields.end(); f; f = f->Prev) {
          if (f->Data != this) { //not a self recursive field
            ComplexType * newField = new ComplexType(*f->Data);
            complexfields.push_front(newField);
            setParent(this, newField);
          } else {
            //Self recursive field
            ComplexType * field = new ComplexType(this);
            field->name.upload(f->Data->getName().convertedValue);
            field->applyReference(*f->Data);
            field->type.upload(ct->getName().convertedValue + Mstring(".") + f->Data->getName().convertedValue);
            field->type.no_replace = true;
            field->setMinOccurs(f->Data->getMinOccurs());
            field->setMaxOccurs(f->Data->getMaxOccurs());
            complexfields.push_front(field);
            setParent(this, field);
          }
        }
      } else {
        //Normal extension
        for (List<ComplexType*>::iterator f = ct->complexfields.end(); f; f = f->Prev) {
          ComplexType * newField = new ComplexType(*f->Data);
          complexfields.push_front(newField);
          setParent(this, newField);
        }
      }
    }
  }
}

void ComplexType::resolveComplexTypeRestriction() {
  if (mode == restrictionMode && cmode == CT_complextype_mode && !outside_reference.empty()) {
    ComplexType * ct = (ComplexType*) TTCN3ModuleInventory::getInstance().lookup(this, want_CT);
    if (ct == NULL) {
      printError(module->getSchemaname(), name.convertedValue,
        "Reference for a non-defined type: " + getReference().repr());
      TTCN3ModuleInventory::getInstance().incrNumErrors();
      return;
    }
    if(ct->getXsdtype() != n_NOTSET) {
      if (ct->resolved == No) {
        ct->referenceResolving();
      }
      ComplexType * maybeAlias = ct;
      while (maybeAlias != NULL && maybeAlias->getAlias() != NULL) {
        maybeAlias = (ComplexType*)maybeAlias->getAlias();
      }
      bool isModifiedAttr = applyAttributeRestriction(maybeAlias);
      if(!isModifiedAttr && !hasComplexRestriction(maybeAlias)){
        type.upload(ct->getName().convertedValue);
        
        for (List<ComplexType*>::iterator field = complexfields.begin(); field; field = field->Next) {
          field->Data->setInvisible();
        }
        
        for (List<AttributeType*>::iterator field = attribfields.begin(); field; field = field->Next) {
          field->Data->setInvisible();
        }
        ct->addToNameDepList(this);
        nameDep = ct;
        alias = ct;
        return;
      }
      
      outside_reference.set_resolved(ct);

      size_t size = complexfields.size();
      size_t i = 0;
      List<ComplexType*>::iterator field = complexfields.begin();
      for (; i < size; field = field->Next, i = i + 1){
        List<ComplexType*>::iterator field2 = ct->complexfields.begin();
        for (; field2; field2 = field2->Next) {
          if (field->Data->getName().convertedValue == field2->Data->getName().convertedValue &&
            field->Data->getType().convertedValue.getValueWithoutPrefix(':') == field2->Data->getType().convertedValue.getValueWithoutPrefix(':') &&
            field->Data->complexfields.size() <= field2->Data->complexfields.size() &&
            hasMatchingFields(field->Data->complexfields, field2->Data->complexfields)) {
            // TODO: better algorithm to find matching fields
            field->Data->applyReference(*field2->Data, false);
            break;
          }
        }
        if(field2 == NULL){
          field->Data->setInvisible();
        }
      }
    }
  }
}

bool ComplexType::hasMatchingFields(const List<ComplexType*>& mainList, const List<ComplexType*>& subList) const {
    List<ComplexType*>::iterator field = mainList.begin();
    for (; field; field = field->Next){
      List<ComplexType*>::iterator field2 = subList.begin();
      bool found = false;
      for (; field2; field2 = field2->Next) {
        if(field->Data->getName().convertedValue == field2->Data->getName().convertedValue &&
          field->Data->getType().convertedValue == field2->Data->getType().convertedValue) {
            found = true;
            break;
        }
      }
      if(!found) {
          return false;
      }
    }
    return true;
}

bool ComplexType::hasComplexRestriction(ComplexType* ct) const {
  if(complexfields.size() != ct->complexfields.size() || hasRestrictionOrExtension()) {
    return true;
  }
  
  for(List<ComplexType*>::iterator field = complexfields.begin(); field; field = field->Next) {
    for(List<ComplexType*>::iterator field2 = ct->complexfields.begin(); field2; field2 = field2->Next) {
      if(field->Data->getName().convertedValue.getValueWithoutPrefix(':') == field2->Data->getName().convertedValue.getValueWithoutPrefix(':') &&
         field->Data->getType().convertedValue.getValueWithoutPrefix(':') == field2->Data->getType().convertedValue.getValueWithoutPrefix(':')) {
        if(field->Data->getMinOccurs() == field2->Data->getMinOccurs() && field->Data->getMaxOccurs() == field2->Data->getMaxOccurs()){
          if(field->Data->hasComplexRestriction(field2->Data)) {
            return true;
          }
        } else {
          return true;
        }
      }
    }
  }
  return false;
}

void ComplexType::resolveUnion(SimpleType *st) {
  if (parent != NULL && parent->with_union && xsdtype == n_simpleType && !outside_reference.empty()) {
    if (st->getXsdtype() != n_NOTSET) {
      ComplexType * ct = (ComplexType*) st;
      outside_reference.set_resolved(ct);
      for (List<ComplexType*>::iterator field = ct->complexfields.begin(); field; field = field->Next) {
        ComplexType * newField = new ComplexType(*field->Data);
        parent->complexfields.push_back(newField);
        setParent(parent, newField);
      }
      setInvisible();
    }
  }
}

void ComplexType::modifyAttributeParent() {
  if (nillable_field != NULL) {
    ((ComplexType*) nillable_field)->actfield = nillable_field;
  } else {
    actfield = this;
  }
}

//Element substitution
void ComplexType::addSubstitution(SimpleType * st){
  ComplexType * element;
  if(st->getXsdtype() == n_NOTSET || !complexfields.empty()){
    element = new ComplexType(*st, fromTagSubstitution);
  }else {
    element = new ComplexType(*(ComplexType*)st);
    element->variant.clear();
  }
  element->subsGroup = this;
  element->parent = this;
  if(complexfields.empty()){ //The first element(head) is the st
    element->setTypeValue(st->getType().convertedValue);
    if(st->hasVariant(Mstring("\"abstract\""))){
      element->addVariant(V_abstract);
    }
    if(st->getReference().get_ref() != NULL){
      ((SimpleType*)st->getReference().get_ref())->addToNameDepList(element);
      nameDep = ((SimpleType*)st->getReference().get_ref());
    }
    module->addElementType(element->getType().convertedValue, element);
    element->addVariant(V_formAs, Mstring("qualified"));
  }else {
    Mstring newType;
    if(st->getType().convertedValue == "anyType"){
      newType = complexfields.front()->getType().convertedValue;
    }else {
      newType = st->getName().convertedValue;
      st->addToNameDepList(element);
      element->nameDep = st;
    }
    element->setTypeValue(newType);
    BlockValue front_block = complexfields.front()->getBlock();
    if(front_block == all || front_block == substitution){
      element->addVariant(V_block);
    }else if(front_block == restriction || front_block == extension){
      const Mstring& head_type = complexfields.front()->getType().convertedValue.getValueWithoutPrefix(':');
      //To decide if they came from a common ancestor
      Mstring elem_type = findRoot(front_block, st, head_type, true);
      if(head_type == elem_type){
        element->addVariant(V_block);
      }
    }
  }

  element->setNameValue(st->getName().convertedValue);
  element->top = false;
  complexfields.push_back(element);
}

void ComplexType::addTypeSubstitution(SimpleType * st){
  ComplexType * element;
  if(st->getXsdtype() == n_NOTSET || !complexfields.empty()){
    element = new ComplexType(*st, fromTypeSubstitution);
  }else {
    //Only need a plain complextype
    //Head element
    element = new ComplexType(this);
    //Just the block needed from st
    element->block = st->getBlock();
  }
  st->addToNameDepList(element);
  element->nameDep = st;
  element->typeSubsGroup = this;
  element->parent = this;
  if(complexfields.empty()){ //The first element(head) is the st
    if(st->hasVariant(Mstring("\"abstract\""))){
      element->addVariant(V_abstract);
    }
  }else {
    BlockValue front_block = complexfields.front()->getBlock();
    if(front_block == all){
      element->addVariant(V_block);
    }else if(front_block == restriction || front_block == extension){
      const Mstring& head_type = complexfields.front()->getType().convertedValue.getValueWithoutPrefix(':');
      //To decide if they came from a common ancestor
      Mstring elem_type = findRoot(front_block, st, head_type, true);
      if(head_type == elem_type){
        element->addVariant(V_block);
      }
    }
  }
  //Cascading to parent type substitution
  if(parentTypeSubsGroup != NULL && !complexfields.empty()){
    parentTypeSubsGroup->addTypeSubstitution(st);
  }
  element->top = false;
  complexfields.push_back(element);
  element->setTypeValue(st->getName().convertedValue.getValueWithoutPrefix(':'));
  element->setNameValue(st->getName().convertedValue.getValueWithoutPrefix(':'));
}

Mstring ComplexType::findRoot(const BlockValue block_value, SimpleType* elem, const Mstring& head_type, const bool first){
  const Mstring elemName = elem->getName().convertedValue.getValueWithoutPrefix(':');
  const Mstring elemType = elem->getType().convertedValue.getValueWithoutPrefix(':');

  if(!first && !isFromRef() && elemType == head_type){
    return elemType;
  }else if((isFromRef() &&
          ((elem->getMode() == restrictionMode && block_value == restriction) ||
           (elem->getMode() == extensionMode && block_value == extension))) && elemType == head_type){
    return elemType;
  }else if(!first && elemName == head_type){
    return elemName;
  }else {
    SimpleType * st = NULL;
    if((elem->getMode() == restrictionMode && block_value == restriction) ||
       (elem->getMode() == extensionMode && block_value == extension)){
      if(!elem->getReference().is_resolved()){
        elem->referenceResolving();
      }
      if(elem->getXsdtype() != n_NOTSET){
        ComplexType * ct = (ComplexType*)elem;
        if(ct->basefield != NULL && ct->basefield->getType().convertedValue.getValueWithoutPrefix(':') == head_type){
          return head_type;
        }else if(ct->basefield != NULL){
          st = (SimpleType*)TTCN3ModuleInventory::getInstance().lookup(ct->basefield, want_BOTH);
        }
      }
      if(st == NULL){
        st = (SimpleType*)(elem->getReference().get_ref());
      }
    }else if(elem->getMode() == noMode && (block_value == restriction || block_value == extension)){
      st = (SimpleType*)TTCN3ModuleInventory::getInstance().lookup(this, elem->getType().convertedValue, want_BOTH);
    }
    if(st != NULL && elem != st){
      return findRoot(block_value, st, head_type, false);
    }
  }
  if(elem->getMode() == noMode && !first){
    return elemType;
  }else {
    return empty_string;
  }
}

