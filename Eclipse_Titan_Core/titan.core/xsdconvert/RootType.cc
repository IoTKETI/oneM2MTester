/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Delic, Adam
 *   Raduly, Csaba
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
#include "RootType.hh"

#include "TTCN3Module.hh"

RootType::RootType(XMLParser * a_parser, TTCN3Module * a_module, const ConstructType a_construct)
: parser(a_parser)
, module(a_module)
, name()
, type(a_module, a_parser)
, variant()
, variant_ref()
, comment()
, construct(a_construct)
, origin(from_unknown)
, visible(true)
, nameDepList()
, minOccurs(1)
, maxOccurs(1)
, min_mod(false)
, max_mod(false){
  switch (a_construct) {
    case c_schema:
    case c_annotation:
    case c_include:
    case c_import:
      //redundant: origin = from_unknown;
      break;
    case c_unknown: // because when using fields in complextypes we set construct to c_unknown
    case c_simpleType:
      origin = from_simpleType;
      type.upload(Mstring("anySimpleType"), false);
      break;
    case c_element:
      origin = from_element;
      type.upload(Mstring("anyType"), false);
      addVariant(V_element);
      break;
    case c_attribute:
      origin = from_attribute;
      type.upload(Mstring("anySimpleType"), false);
      addVariant(V_attribute);
      break;
    case c_complexType:
      origin = from_complexType;
      type.upload(Mstring("record"), false);
      break;
    case c_group:
      origin = from_group;
      type.upload(Mstring("record"), false);
      addVariant(V_untagged);
      break;
    case c_attributeGroup:
      origin = from_attributeGroup;
      type.upload(Mstring("record"), false);
      addVariant(V_attributeGroup);
      visible = false;
      break;
    default:
      break;
  }
}

void RootType::addVariant(const VariantMode var, const Mstring& var_value, const bool into_variant_ref) {
  Mstring variantstring;

  switch (var) {
    case V_abstract:
      variantstring = "\"abstract\"";
      break;
    case V_anyAttributes:
      variantstring = "\"anyAttributes" + var_value + "\"";
      break;
    case V_anyElement:
      variantstring = "\"anyElement" + var_value + "\"";
      break;
    case V_attribute:
      variantstring = "\"attribute\"";
      break;
    case V_attributeFormQualified:
      variantstring = "\"attributeFormQualified\"";
      break;
    case V_attributeGroup:
      variantstring = "\"attributeGroup\"";
      break;
    case V_block:
      variantstring = "\"block\"";
      break;
    case V_controlNamespace:
      variantstring = "\"controlNamespace" + var_value + "\"";
      break;
    case V_defaultForEmpty:
      // The apostrophes are not needed because the var_value will be a constant
      // reference.
      variantstring = "\"defaultForEmpty as " + var_value + "\""; // chapter 7.1.5
      break;
    case V_element:
      variantstring = "\"element\"";
      break;
    case V_elementFormQualified:
      variantstring = "\"elementFormQualified\"";
      break;
    case V_embedValues:
      variantstring = "\"embedValues\"";
      break;
    case V_formAs:
      variantstring = "\"form as " + var_value + "\"";
      break;
    case V_list:
      variantstring = "\"list\"";
      break;
    case V_nameAs:
      variantstring = "\"name as \'" + var_value + "\'\"";
      break;
    case V_namespaceAs:
    {
      Mstring prefix;
      Mstring uri;
      for (List<NamespaceType>::iterator namesp = module->getDeclaredNamespaces().begin(); namesp; namesp = namesp->Next) {
        if (namesp->Data.uri == var_value) {
          prefix = namesp->Data.prefix;
          uri = namesp->Data.uri;
          break;
        }
      }
      if (prefix.empty() || uri.empty()) {
        break;
      }
      variantstring = "\"namespace as \'" + uri + "\' prefix \'" + prefix + "\'\"";
      break;
    }
    case V_onlyValue:
      variantstring = var_value;
      break;
    case V_onlyValueHidden:
      hidden_variant.push_back(var_value);
      break;
    case V_untagged:
      variantstring = "\"untagged\"";
      break;
    case V_useNil:
      variantstring = "\"useNil\"";
      break;
    case V_useNumber:
      variantstring = "\"useNumber\"";
      break;
    case V_useOrder:
      variantstring = "\"useOrder\"";
      break;
    case V_useType:
      variantstring = "\"useType\"";
      break;
    case V_useUnion:
      variantstring = "\"useUnion\"";
      break;
    case V_whiteSpace:
      variantstring = "\"whiteSpace " + var_value + "\"";
      break;
    case V_fractionDigits:
      variantstring = "\"fractionDigits " + var_value + "\"";
      break;
  }

  if (!variantstring.empty()) {
    variant.push_back(variantstring);
    if (into_variant_ref) {
      variant_ref.push_back(variantstring);
    }
  }
}

void RootType::printVariant(FILE * file) {
  if (!e_flag_used && !variant.empty()) {
    fprintf(file, "\nwith {\n");
    for (List<Mstring>::iterator var = variant.end(); var; var = var->Prev) {
      fprintf(file, "  variant %s;\n", var->Data.c_str());
    }
    for (List<Mstring>::iterator var = hidden_variant.end(); var; var = var->Prev) {
      fprintf(file, "  //variant %s;\n", var->Data.c_str());
    }
    fprintf(file, "}");
  } else if (!e_flag_used && type.originalValueWoPrefix == Mstring("boolean")) {
    fprintf(file, ";\n//with {\n");
    for (List<Mstring>::iterator var = hidden_variant.end(); var; var = var->Prev) {
      fprintf(file, "  //variant %s;\n", var->Data.c_str());
    }
    fprintf(file, "//}");
  }
}

void RootType::addComment(const Mstring& text) {
  comment.push_back(Mstring("/* " + text + " */\n"));
}

void RootType::printComment(FILE * file, unsigned int level) {
  if (!c_flag_used && !comment.empty()) {
    for (List<Mstring>::iterator c = comment.begin(); c; c = c->Next) {
      indent(file, level);
      fprintf(file, "%s", c->Data.c_str());
    }
  }
}

void RootType::printMinOccursMaxOccurs(FILE * file, const bool inside_union,
  const bool empty_allowed /* = true */) const {

  unsigned long long tmp_minOccurs = minOccurs;
  if (minOccurs == 0 && !empty_allowed) tmp_minOccurs = 1ULL;

  if (maxOccurs == 1) {
    if (minOccurs == 0) {
      if (inside_union || name.list_extension) {
        fputs("record length(", file);
        if (empty_allowed) fputs("0 .. ", file);
        // else: length(1..1) is shortened to length(1)
        fputs("1) of ", file);
      }
      // else it's optional which is not printed from here
    } else if (minOccurs == 1) {
      // min==max==1; do nothing unless...
      if (name.convertedValue == "embed_values") {
        fputs("record length(1) of ", file);
      }
    }
  } else if (maxOccurs == ULLONG_MAX) {
    if (minOccurs == 0) {
      fputs("record ", file);
      if (!empty_allowed) fputs("length(1 .. infinity) ", file);
      fputs("of ", file);
    } else fprintf(file, "record length(%llu .. infinity) of ", tmp_minOccurs);
  } else {
    if (tmp_minOccurs == maxOccurs) {
      fprintf(file, "record length(%llu) of ", tmp_minOccurs);
    } else {
      fprintf(file, "record length(%llu .. %llu) of ", tmp_minOccurs, maxOccurs);
    }
  }
}

bool RootType::hasVariant(const Mstring& var) const{
  for(List<Mstring>::iterator vars = variant.begin(); vars; vars = vars->Next){
    if(vars->Data.isFound(var)){
      return true;
    }
  }
  for(List<Mstring>::iterator vars = hidden_variant.begin(); vars; vars = vars->Next){
    if(vars->Data.isFound(var)){
      return true;
    }
  }
  return false;
}

void TypeType::upload(const Mstring& input, bool prefixCheck) {
  if (input.empty()) return;
  convertedValue = input;
  originalValueWoPrefix = input.getValueWithoutPrefix(':');
  if (isBuiltInType(input)) {
    refPrefix = input.getPrefix(':');
    if (prefixCheck) {
      checkBuintInTypeReference();
    }
  }
}

void TypeType::checkBuintInTypeReference() {
  bool found = false;
  for (List<Mstring>::iterator px = t_module->getxmlSchemaPrefixes().begin(); px; px = px->Next) {
    if (refPrefix == px->Data) {
      found = true;
      break;
    }
  }
  // Second chance. It may be a prefix from one of the xsd-s.
  if (!found) {
    for (List<NamespaceType>::iterator ns = t_module->getDeclaredNamespaces().begin(); ns; ns = ns->Next) {
      if (refPrefix == ns->Data.prefix.c_str()) {
        found = true;
        break;
      }
    }
  }
  if (!found) {
    printError(t_module->getSchemaname(), t_parser->getActualLineNumber(), Mstring("Cannot find the namespace of type: ") + originalValueWoPrefix);
    t_parser->incrNumErrors();
  }
}
