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
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
#include "XMLParser.hh"
#include "TTCN3Module.hh"
#include "SimpleType.hh"
#include "ComplexType.hh"
#include "converter.hh"

#include "GeneralFunctions.hh"

#include <cstring> // for using "memset" function
#include <cstdio>
#include <cerrno>
#include <climits>
#ifndef ULLONG_MAX
#define ULLONG_MAX 18446744073709551615ULL
#endif
#ifndef LLONG_MIN
#define LLONG_MIN -9223372036854775808LL
#endif
#ifndef LLONG_MAX
#define LLONG_MAX 9223372036854775807LL
#endif

extern bool n_flag_used;
extern bool V_flag_used;
extern bool w_flag_used;
extern bool x_flag_used;

bool XMLParser::suspended = false;
unsigned int XMLParser::num_errors = 0;
unsigned int XMLParser::num_warnings = 0;

XMLParser::XMLParser(const char * a_filename)
: module(NULL) // get value with 'connectWithModule()' method
, filename(a_filename) // includes the path of the file
, parser(NULL)
, context(NULL)
, parserCheckingXML(NULL)
, contextCheckingXML(NULL)
, contextCheckingXSD(NULL)
, actualDepth(0)
, actualTagName(n_NOTSET)
, actualTagAttributes(this)
, parentTagNames()
, inside_annotation()
, lastWasListEnd(false){
  xmlSetExternalEntityLoader(xmlNoNetExternalEntityLoader);

  parserCheckingXML = (xmlSAXHandler *) malloc(sizeof (xmlSAXHandler));
  memset(parserCheckingXML, 0, sizeof (xmlSAXHandler));
  parserCheckingXML->initialized = XML_SAX2_MAGIC;
  parserCheckingXML->warning = warningHandler;
  parserCheckingXML->error = errorHandler;
  if (n_flag_used) {
    parserCheckingXML->startElementNs = (startElementNsSAX2Func) wrapper_to_call_startelement_when_xsd_read_h;
  }
  contextCheckingXML = xmlCreateFileParserCtxt(a_filename);
  if (!contextCheckingXML) {
    fprintf(stderr,
      "ERROR:\n"
      "Creating XML syntax checker has failed.\n");
    ++num_errors;
    return;
  }
  contextCheckingXML->sax = parserCheckingXML;
  if (n_flag_used) {
    contextCheckingXML->userData = this;
  }

  if (!x_flag_used) {
    contextCheckingXSD = xmlSchemaNewParserCtxt(a_filename);
    if (!contextCheckingXSD) {
      fprintf(stderr,
        "ERROR:\n"
        "Creating XSD validator has failed.\n");
      ++num_errors;
      return;
    }
    xmlSchemaSetParserErrors(contextCheckingXSD, errorHandler, warningHandler, 0);
  }

  parser = (xmlSAXHandler *) malloc(sizeof (xmlSAXHandler));
  memset(parser, 0, sizeof (xmlSAXHandler));
  parser->initialized = XML_SAX2_MAGIC;
  parser->startElementNs = (startElementNsSAX2Func) wrapper_to_call_startelement_h;
  parser->endElementNs = (endElementNsSAX2Func) wrapper_to_call_endelement_h;
  parser->characters = (charactersSAXFunc) wrapper_to_call_characterdata_h;
  parser->comment = (commentSAXFunc) wrapper_to_call_comment_h;

  context = xmlCreateFileParserCtxt(filename.c_str());
  if (!context) {
    fprintf(stderr,
      "ERROR:\n"
      "Creating parser for file '%s' has failed.\n", filename.c_str());
    ++num_errors;
    return;
  }
  context->sax = parser;
  context->userData = this;
}

XMLParser::~XMLParser() {
  context->sax = NULL;
  xmlFreeDoc(context->myDoc);
  contextCheckingXML->sax = NULL;
  free(parser);
  free(parserCheckingXML);
  if (context) {
    xmlFreeParserCtxt(context);
  }
  if (contextCheckingXML) {
    xmlFreeParserCtxt(contextCheckingXML);
  }
  if (contextCheckingXSD) {
    xmlSchemaFreeParserCtxt(contextCheckingXSD);
  }
}

void XMLParser::checkSyntax() {
  xmlParseDocument(contextCheckingXML);
}

void XMLParser::validate() {
  if (!x_flag_used && !n_flag_used) {
    xmlSchemaPtr schema = xmlSchemaParse(contextCheckingXSD);
    if (schema) {
      xmlSchemaValidCtxtPtr validator = xmlSchemaNewValidCtxt(schema);
      if (validator) {
        // do not use this->context!
        xmlParserCtxtPtr newcontext = xmlNewParserCtxt();
        xmlDocPtr doc = xmlCtxtReadFile(newcontext, filename.c_str(), NULL, 0);
        if (doc) {
          // Don't try this, it always fails
          //int result = xmlSchemaValidateDoc(validator, doc);
          //(void)result; // 0=ok, errorcode > 0, intrnal error == -1
          xmlFreeDoc(doc);
        }
        xmlSchemaFreeValidCtxt(validator);
        xmlFreeParserCtxt(newcontext);
      }
      xmlSchemaFree(schema);
    }
  }
}

void XMLParser::startConversion(TTCN3Module * a_module) {
  module = a_module;
  xmlParseDocument(context);
}

void XMLParser::wrapper_to_call_startelement_h(XMLParser *self, const xmlChar * localname, const xmlChar *, const xmlChar *,
  int nb_namespaces, const xmlChar ** namespaces, const int nb_attributes, int, const xmlChar ** attributes) {
  self->startelementHandler(localname, nb_namespaces, namespaces, nb_attributes, attributes);
}

void XMLParser::wrapper_to_call_endelement_h(XMLParser *self, const xmlChar * localname, const xmlChar *, const xmlChar *) {
  self->endelementHandler(localname);
}

void XMLParser::wrapper_to_call_comment_h(XMLParser *self, const xmlChar * value) {
  self->commentHandler(value);
}

void XMLParser::wrapper_to_call_characterdata_h(XMLParser *self, const xmlChar * ch, int len) {
  self->characterdataHandler(ch, len);
}

void XMLParser::wrapper_to_call_startelement_when_xsd_read_h(XMLParser *self, const xmlChar * localname, const xmlChar *, const xmlChar *,
  int nb_namespaces, const xmlChar ** namespaces, const int nb_attributes, int, const xmlChar ** attributes) {
  self->startelementHandlerWhenXSDRead(localname, nb_namespaces, namespaces, nb_attributes, attributes);
}

void XMLParser::warningHandler(void *, const char *, ...) {
  if (w_flag_used) {
    return;
  }

  xmlErrorPtr error = xmlGetLastError();

  if (error->file == NULL) {
    fprintf(stderr,
      "WARNING:\n"
      "%s",
      error->message);
  } else {
    fprintf(stderr,
      "WARNING:\n"
      "%s (in line %d): "
      "%s",
      error->file,
      error->line,
      error->message);
    ++num_warnings;
  }
}

void XMLParser::errorHandler(void *, const char *, ...) {
  xmlErrorPtr error = xmlGetLastError();

  if (error->code == XML_SCHEMAP_SRC_RESOLVE) {
    return;
  }
  if (error->code == XML_SCHEMAP_COS_ALL_LIMITED) {
    return;
  }

  switch (error->level) {
    case XML_ERR_ERROR:
      fputs("ERROR:\n", stderr);
      break;
    case XML_ERR_FATAL:
      fputs("FATAL ERROR:\n", stderr);
      break;
    default: // warning or no error, can't happen (famous last words)
      break;
  }

  if (error->file != NULL) {
    fprintf(stderr, "%s (in line %d): ", error->file, error->line);
  }

  fputs(error->message, stderr); // libxml2 supplies a trailing \n
  ++num_errors;
}

void XMLParser::startelementHandler(const xmlChar * localname,
  int nb_namespaces, const xmlChar ** namespaces, int nb_attributes, const xmlChar ** attributes) {
  fillUpActualTagName((const char *) localname, startElement);
  fillUpActualTagAttributes((const char **) attributes, nb_attributes);

  switch (module->getActualXsdConstruct()) {
    case c_unknown:
    {
      switch (actualTagName) {
        case n_schema:
        {
          module->setActualXsdConstruct(c_schema);

          module->loadValuesFromXMLDeclaration((const char *) context->version,
            (const char *) context->encoding, context->standalone);

          List<NamespaceType> declaredNamespaces;
          for (int i = 0; i < nb_namespaces * 2; i = i + 2) {
            NamespaceType tmp_ns_pair;

            if (namespaces[i] != NULL) {
              tmp_ns_pair.prefix = (const char*) namespaces[i];
            }
            // else leave it as empty string

            if (namespaces[i + 1] != NULL) {
              tmp_ns_pair.uri = (const char*) namespaces[i + 1];
            }
            // else leave it as empty string

            declaredNamespaces.push_back(tmp_ns_pair);
          }

          module->loadValuesFromSchemaTag(actualTagAttributes.targetNamespace, declaredNamespaces,
            actualTagAttributes.elementFormDefault, actualTagAttributes.attributeFormDefault,
            actualTagAttributes.blockDefault);
          // Only need the targetNamespace. Stop.
          if (n_flag_used) {
            xmlStopParser(context);
          }
          break;
        }
        default:
          break;
      }
      break;
    }

    case c_schema:
    {
      switch (actualTagName) {
        case n_simpleType:
          module->addMainType(c_simpleType);
          break;
        case n_element:
          module->addMainType(c_element);
          break;
        case n_attribute:
          module->addMainType(c_attribute);
          break;
        case n_complexType:
          module->addMainType(c_complexType);
          break;
        case n_group:
          module->addMainType(c_group);
          break;
        case n_attributeGroup:
          module->addMainType(c_attributeGroup);
          break;
        case n_include:
          module->addMainType(c_include);
          break;
        case n_import:
          module->addMainType(c_import);
          break;
        default:
          break;
      }
      break;
    }

    default:
      if (module->hasDefinedMainType()) {
        if (lastWasListEnd) {
          ((SimpleType&)(module->getLastMainType())).modifyList();
          lastWasListEnd = false;
        }
        if(actualTagName == n_annotation ||
          actualTagName == n_appinfo ||
          actualTagName == n_documentation){
          inside_annotation.push_back(actualTagName);
          module->getLastMainType().loadWithValues();
        }else if(inside_annotation.empty()){
          module->getLastMainType().loadWithValues();
        }
      }
      break;
  }

  //Standard section 7.1.1
  if (!actualTagAttributes.id.empty()) {
    ConstructType type = module->getActualXsdConstruct();
    module->addMainType(c_idattrib);
    module->setActualXsdConstruct(type);
  }

  ++actualDepth;
  parentTagNames.push_back(actualTagName);
  if (lastWasListEnd) {
    lastWasListEnd = false;
  }
}

void XMLParser::startelementHandlerWhenXSDRead(const xmlChar * localname,
  int /*nb_namespaces*/, const xmlChar ** /*namespaces*/, int /*nb_attributes*/, const xmlChar ** /*attributes*/) {
  if (strcmp((const char*)localname, "schema") == 0) {
    // Don't parse beyond the schema tag.
    xmlStopParser(contextCheckingXML);
  }
}

void XMLParser::endelementHandler(const xmlChar * localname) {
  fillUpActualTagName((const char *) localname, endElement);

  bool modify = false;
  TagName tag = parentTagNames.back();
  //After some tags there is no need to call modifyValues
  if (tag == n_element ||
    tag == n_all ||
    tag == n_choice ||
    tag == n_group ||
    tag == n_attributeGroup ||
    tag == n_extension ||
    tag == n_simpleType ||
    tag == n_simpleContent ||
    tag == n_sequence ||
    tag == n_complexType ||
    tag == n_complexContent ||
    tag == n_attribute ||
    tag == n_anyAttribute
    ) {
    modify = true;
  }

  if(tag == n_annotation ||
     tag == n_appinfo ||
     tag == n_documentation){
    inside_annotation.pop_back();
  }
  
  if(tag == n_list) {
    lastWasListEnd = true;
    if(module->hasDefinedMainType()) {
      SimpleType& st = (SimpleType&)(module->getLastMainType());
      if(st.getXsdtype() == n_NOTSET){
        st.setMode(SimpleType::restrictionAfterListMode);
      }
    }
  } else if (tag != n_simpleType){
    lastWasListEnd = false;
  }

  --actualDepth;
  if (actualDepth == 0 || actualDepth == 1) {
    module->setActualXsdConstruct(c_schema);
  }

  if (module->hasDefinedMainType() && modify) {
    module->getLastMainType().modifyValues();
  }
  parentTagNames.pop_back();
}

void XMLParser::commentHandler(const xmlChar * text) {
  Mstring comment((const char *) text);
  comment.removeWSfromBegin();
  comment.removeWSfromEnd();
  if (comment.empty()) {
    return;
  }

  if (module->getActualXsdConstruct() == c_schema) {
    module->addMainType(c_annotation);
    module->setActualXsdConstruct(c_schema); // actualXsdConstruct was set to c_annotation
  }

  if (module->hasDefinedMainType()) {
    module->getLastMainType().addComment(comment);
  }
}

void XMLParser::characterdataHandler(const xmlChar * text, const int length) {
  if (suspended) {
    return;
  }

  char * temp = (char *) Malloc(length + 1);
  memcpy(temp, text, length);
  temp[length] = '\0';
  Mstring comment(temp);
  Free(temp);

  comment.removeWSfromBegin();
  comment.removeWSfromEnd();
  if (comment.empty()) {
    return;
  }

  if (module->getActualXsdConstruct() == c_schema) {
    module->addMainType(c_annotation);
  }
  if (module->hasDefinedMainType()) {
    module->getLastMainType().addComment(comment);
  }
}

void XMLParser::fillUpActualTagName(const char * localname, const tagMode mode) {
  Mstring name_s(localname);

  if (name_s == "all")
    actualTagName = n_all;
  else if (name_s == "annotation")
    actualTagName = n_annotation;
  else if (name_s == "any")
    actualTagName = n_any;
  else if (name_s == "anyAttribute")
    actualTagName = n_anyAttribute;
  else if (name_s == "appinfo") {
    actualTagName = n_appinfo;
    switch (mode) {
      case startElement:
        suspended = true;
        break;
      case endElement:
        suspended = false;
        break;
      default:
        break;
    }
  } else if (name_s == "attribute")
    actualTagName = n_attribute;
  else if (name_s == "attributeGroup")
    actualTagName = n_attributeGroup;
  else if (name_s == "choice")
    actualTagName = n_choice;
  else if (name_s == "complexContent")
    actualTagName = n_complexContent;
  else if (name_s == "complexType")
    actualTagName = n_complexType;
  else if (name_s == "definition")
    actualTagName = n_definition;
  else if (name_s == "documentation")
    actualTagName = n_documentation;
  else if (name_s == "element")
    actualTagName = n_element;
  else if (name_s == "enumeration")
    actualTagName = n_enumeration;
  else if (name_s == "extension")
    actualTagName = n_extension;
  else if (name_s == "field") {
    actualTagName = n_field;
    if (mode == startElement) {
      printWarning(filename, xmlSAX2GetLineNumber(context),
        Mstring("The 'field' tag is ignored by the standard."));
      ++num_warnings;
    }
  } else if (name_s == "fractionDigits") {
    actualTagName = n_fractionDigits;
  } else if (name_s == "group")
    actualTagName = n_group;
  else if (name_s == "import")
    actualTagName = n_import;
  else if (name_s == "include")
    actualTagName = n_include;
  else if (name_s == "key") {
    actualTagName = n_key;
    if (mode == startElement) {
      printWarning(filename, xmlSAX2GetLineNumber(context),
        Mstring("The 'key' tag is ignored by the standard."));
      ++num_warnings;
    }
  } else if (name_s == "keyref") {
    actualTagName = n_keyref;
    if (mode == startElement) {
      printWarning(filename, xmlSAX2GetLineNumber(context),
        Mstring("The 'keyref' tag ignored by the standard."));
      ++num_warnings;
    }
  } else if (name_s == "length")
    actualTagName = n_length;
  else if (name_s == "label")
    actualTagName = n_label;
  else if (name_s == "list")
    actualTagName = n_list;
  else if (name_s == "maxExclusive")
    actualTagName = n_maxExclusive;
  else if (name_s == "maxInclusive")
    actualTagName = n_maxInclusive;
  else if (name_s == "maxLength")
    actualTagName = n_maxLength;
  else if (name_s == "minExclusive")
    actualTagName = n_minExclusive;
  else if (name_s == "minInclusive")
    actualTagName = n_minInclusive;
  else if (name_s == "minLength")
    actualTagName = n_minLength;
  else if (name_s == "notation")
    ;
  else if (name_s == "pattern")
    actualTagName = n_pattern;
  else if (name_s == "redefine")
    actualTagName = n_redefine;
  else if (name_s == "restriction")
    actualTagName = n_restriction;
  else if (name_s == "schema")
    actualTagName = n_schema;
  else if (name_s == "selector") {
    actualTagName = n_selector;
    if (mode == startElement) {
      printWarning(filename, xmlSAX2GetLineNumber(context),
        Mstring("The 'selector' tag ignored by the standard."));
      ++num_warnings;
    }
  } else if (name_s == "sequence")
    actualTagName = n_sequence;
  else if (name_s == "simpleContent")
    actualTagName = n_simpleContent;
  else if (name_s == "simpleType")
    actualTagName = n_simpleType;
  else if (name_s == "totalDigits")
    actualTagName = n_totalDigits;
  else if (name_s == "union")
    actualTagName = n_union;
  else if (name_s == "unique") {
    actualTagName = n_unique;
    if (mode == startElement) {
      printWarning(filename, xmlSAX2GetLineNumber(context),
        Mstring("The 'unique' tag ignored by the standard."));
      ++num_warnings;
    }
  } else if (name_s == "whiteSpace")
    actualTagName = n_whiteSpace;
}

void XMLParser::fillUpActualTagAttributes(const char ** attributes, const int att_count) {

  struct attribute_data {
    const char * name;
    const char * prefix;
    const char * uri;
    const char * value_start;
    const char * value_end;
  };
  attribute_data * ad = (attribute_data *) attributes;

  Mstring * att_name_s = new Mstring[att_count];
  Mstring * att_value_s = new Mstring[att_count];
  TagAttributeName * att_name_e = new TagAttributeName[att_count];

  for (int i = 0; i != att_count; ++i) {
    att_name_s[i] = ad[i].name;
    att_value_s[i] = Mstring(ad[i].value_end - ad[i].value_start, ad[i].value_start);

    att_name_e[i] = a_NOTSET;
    if (att_name_s[i] == "abstract") {
      att_name_e[i] = a_abstract;
    } else if (att_name_s[i] == "attributeFormDefault")
      att_name_e[i] = a_attributeFormDefault;
    else if (att_name_s[i] == "base")
      att_name_e[i] = a_base;
    else if (att_name_s[i] == "block") {
      att_name_e[i] = a_block;
    } else if (att_name_s[i] == "blockDefault"){
      att_name_e[i] = a_blockDefault;
    } else if (att_name_s[i] == "default")
      att_name_e[i] = a_default;
    else if (att_name_s[i] == "elementFormDefault")
      att_name_e[i] = a_elementFormDefault;
    else if (att_name_s[i] == "final") {
      att_name_e[i] = a_final; // no effect on the output
    } else if (att_name_s[i] == "finalDefault")
      ;
    else if (att_name_s[i] == "fixed")
      att_name_e[i] = a_fixed;
    else if (att_name_s[i] == "form")
      att_name_e[i] = a_form;
    else if (att_name_s[i] == "id")
      att_name_e[i] = a_id;
    else if (att_name_s[i] == "itemType")
      att_name_e[i] = a_itemType;
    else if (att_name_s[i] == "lang")
      ;
    else if (att_name_s[i] == "maxOccurs")
      att_name_e[i] = a_maxOccurs;
    else if (att_name_s[i] == "memberTypes")
      att_name_e[i] = a_memberTypes;
    else if (att_name_s[i] == "minOccurs")
      att_name_e[i] = a_minOccurs;
    else if (att_name_s[i] == "mixed")
      att_name_e[i] = a_mixed;
    else if (att_name_s[i] == "name")
      att_name_e[i] = a_name;
    else if (att_name_s[i] == "namespace")
      att_name_e[i] = a_namespace;
    else if (att_name_s[i] == "nillable")
      att_name_e[i] = a_nillable;
    else if (att_name_s[i] == "processContents") {
      att_name_e[i] = a_processContents;
      // silently ignored
    } else if (att_name_s[i] == "ref")
      att_name_e[i] = a_ref;
    else if (att_name_s[i] == "schemaLocation")
      att_name_e[i] = a_schemaLocation;
    else if (att_name_s[i] == "substitutionGroup") {
      att_name_e[i] = a_substitutionGroup;
    } else if (att_name_s[i] == "targetNamespace")
      att_name_e[i] = a_targetNamespace;
    else if (att_name_s[i] == "type")
      att_name_e[i] = a_type;
    else if (att_name_s[i] == "use")
      att_name_e[i] = a_use;
    else if (att_name_s[i] == "value")
      att_name_e[i] = a_value;
    else if (att_name_s[i] == "version") {
    }
  }
  actualTagAttributes.fillUp(att_name_e, att_value_s, att_count);
  delete [] att_name_s;
  delete [] att_value_s;
  delete [] att_name_e;
}

XMLParser::TagAttributes::TagAttributes(XMLParser * withThisParser)
: parser(withThisParser)
, attributeFormDefault(notset)
, base()
, default_()
, elementFormDefault(notset)
, fixed()
, form(notset)
, id()
, itemType()
, maxOccurs(1)
, memberTypes()
, minOccurs(1)
, mixed(false)
, name()
, namespace_()
, nillable(false)
, ref()
, schemaLocation()
, source()
, targetNamespace()
, type()
, use(optional)
, value() {
}

void XMLParser::TagAttributes::fillUp(TagAttributeName * att_name_e, Mstring * att_value_s, const int att_count) {
  /**
   * Reset
   */
  abstract = false;
  attributeFormDefault = notset;
  base.clear();
  block = not_set,
  blockDefault = not_set,
  default_.clear();
  elementFormDefault = notset;
  fixed.clear();
  form = notset;
  id.clear();
  itemType.clear();
  maxOccurs = 1;
  memberTypes.clear();
  minOccurs = 1;
  mixed = false;
  name.clear();
  namespace_.clear();
  nillable = false;
  ref.clear();
  schemaLocation.clear();
  source.clear();
  substitionGroup = empty_string;
  targetNamespace.clear();
  type.clear();
  use = optional;
  value.clear();
  /**
   * Upload
   */
  for (int i = 0; i != att_count; ++i) {
    switch (att_name_e[i]) {
      case a_abstract: // Not supported by now
        if (att_value_s[i] == "true") {
          abstract = true;
        } else if (att_value_s[i] == "false") {
          abstract = false;
        }
      case a_attributeFormDefault: // qualified | unqualified
        if (att_value_s[i] == "qualified") {
          attributeFormDefault = qualified;
        } else if (att_value_s[i] == "unqualified") {
          attributeFormDefault = unqualified;
        }
        break;
      case a_base: // QName = anyURI + NCName
        base = att_value_s[i];
        break;
      case a_block: // Not supported by now
        if(att_value_s[i] == "#all"){
          block = all;
        }else if(att_value_s[i] == "substitution"){
          block = substitution;
        }else if(att_value_s[i] == "restriction"){
          block = restriction;
        }else if(att_value_s[i] == "extension"){
          block = extension;
        }
        break;
      case a_blockDefault: // Not supported by now
        if(att_value_s[i] == "#all"){
          blockDefault = all;
        }else if(att_value_s[i] == "substitution"){
          blockDefault = substitution;
        }else if(att_value_s[i] == "restriction"){
          blockDefault = restriction;
        }else if(att_value_s[i] == "extension"){
          blockDefault = extension;
        }
        break;
      case a_default: // string
        default_ = att_value_s[i];
        break;
      case a_elementFormDefault:
        if (att_value_s[i] == "qualified") {
          elementFormDefault = qualified;
        } else if (att_value_s[i] == "unqualified") {
          elementFormDefault = unqualified;
        }
        break;
      case a_final: // Not supported by now
        break;
      case a_finalDefault: // Not supported by now
        break;
      case a_fixed: // string
        fixed = att_value_s[i];
        break;
      case a_form: // qualified | unqualified
        if (att_value_s[i] == "qualified") {
          form = qualified;
        } else if (att_value_s[i] == "unqualified") {
          form = unqualified;
        }
        break;
      case a_lang:
        break;
      case a_id: // ID = NCName
        id = att_value_s[i];
        break;
      case a_itemType: // QName = anyURI + NCName /- used in 'list' tag only
        itemType = att_value_s[i];
        break;
      case a_maxOccurs: // nonNegativeinteger or 'unbounded'
        if (att_value_s[i] == "unbounded") {
          maxOccurs = ULLONG_MAX;
        } else {
          maxOccurs = strtoull(att_value_s[i].c_str(), NULL, 0);
        }
        break;
      case a_memberTypes: // list of QNames - used in 'union' tag only
        memberTypes = att_value_s[i];
        break;
      case a_minOccurs: // nonNegativeInteger
        minOccurs = strtoull(att_value_s[i].c_str(), NULL, 0);
        break;
      case a_mixed: // true | false
        if (att_value_s[i] == "true") {
          mixed = true;
        } else if (att_value_s[i] == "false") {
          mixed = false;
        }
        break;
      case a_name: // NCName
        name = att_value_s[i];
        break;
      case a_namespace: // anyURI
        namespace_ = att_value_s[i];
        break;
      case a_nillable: // true | false
        if (att_value_s[i] == "true") {
          nillable = true;
        } else if (att_value_s[i] == "false") {
          nillable = false;
        }
        break;
      case a_processContents: // Not supported by now
        break;
      case a_ref: // QName = anyURI + NCName
        ref = att_value_s[i];
        break;
      case a_schemaLocation: // anyURI
        schemaLocation = att_value_s[i];
        break;
      case a_substitutionGroup:
        substitionGroup = att_value_s[i];
        break;
      case a_targetNamespace: // anyURI
        targetNamespace = att_value_s[i];
        break;
      case a_type: // QName = anyURI + NCName
        type = att_value_s[i];
        break;
      case a_use: // optional | prohibited | required - used in 'use' tag only
        if (att_value_s[i] == "optional") {
          use = optional;
        } else if (att_value_s[i] == "prohibited") {
          use = prohibited;
        } else if (att_value_s[i] == "required") {
          use = required;
        }
        break;
      case a_value: // value of FACETS
        value = att_value_s[i];
        break;
      case a_source:
      case a_xpath:
      case a_version: // Not supported by now
        break;
      case a_NOTSET:
        break;
      default:
        fprintf(stderr, "Unknown TagAttributeName %d\n", att_name_e[i]);
        abort();
        break;
    }
  }
}

