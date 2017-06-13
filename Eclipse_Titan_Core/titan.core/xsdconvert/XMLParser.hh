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
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
#ifndef PARSER_HH_
#define PARSER_HH_

#include "GeneralTypes.hh"
#include "Mstring.hh"
#include "List.hh"

#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/xmlschemas.h>
#include <libxml/xmlschemastypes.h>

class TTCN3Module;

class XMLParser {
public:
  /**
   * List of possible names of XSD tags
   */

  /**
   * List of possible names of XSD tag attributes
   */
  enum TagAttributeName {
    a_abstract, // Not supported by now
    a_attributeFormDefault,
    a_base,
    a_block, // Not supported by now
    a_blockDefault, // Not supported by now
    a_default,
    a_elementFormDefault,
    a_final, // Not supported by now
    a_finalDefault, // Not supported by now
    a_fixed,
    a_form,
    a_id,
    a_itemType,
    a_lang, // Not supported by now
    a_maxOccurs,
    a_memberTypes,
    a_minOccurs,
    a_mixed,
    a_name,
    a_namespace,
    a_nillable,
    a_processContents, // Not supported by now
    a_ref,
    a_schemaLocation,
    a_source, // ???
    a_substitutionGroup, // Not supported by now
    a_targetNamespace,
    a_type,
    a_use,
    a_value,
    a_version, // Not supported by now
    a_xpath, // Not supported by now

    a_NOTSET
  };

  class TagAttributes {
    TagAttributes(const TagAttributes &); // not implemented
    TagAttributes & operator=(const TagAttributes &); // not implemented
  public:
    XMLParser * parser; // not responsibility for the member

    /**
     * Members for storing actual values of attributes of an XML tag
     */
    bool abstract;
    FormValue attributeFormDefault;
    Mstring base;
    BlockValue block;
    BlockValue blockDefault;
    Mstring default_;
    FormValue elementFormDefault;
    Mstring fixed;
    FormValue form;
    Mstring id;
    Mstring itemType;
    unsigned long long int maxOccurs;
    Mstring memberTypes;
    unsigned long long int minOccurs;
    bool mixed;
    Mstring name;
    Mstring namespace_;
    bool nillable;
    Mstring ref;
    Mstring schemaLocation;
    Mstring source;
    Mstring substitionGroup;
    Mstring targetNamespace;
    Mstring type;
    UseValue use;
    Mstring value;

    TagAttributes(XMLParser * withThisParser);
    // Default destructor is used

    /**
     * Clear and fill up object with values of attributes of current XML tag
     */
    void fillUp(TagAttributeName * att_name_e, Mstring * att_value_s, const int att_count);
  };

private:

  enum tagMode {
    startElement,
    endElement
  };

  /**
   * Related TTCN-3 module
   * Information from parsed XML schema is loaded into this module
   *
   * One-to-one relation between XMLParser object and TTCN3Module object
   */
  TTCN3Module * module; // no responsibility for this member

  /**
   * Name of XML schema
   * Each schema file has a unique XMLParser object to parse it
   */
  Mstring filename;

  /**
   * Pointers that are used by LibXML SAX parser
   */
  xmlSAXHandlerPtr parser;
  xmlParserCtxtPtr context;

  xmlSAXHandlerPtr parserCheckingXML;
  xmlParserCtxtPtr contextCheckingXML;
  xmlSchemaParserCtxtPtr contextCheckingXSD;

  /**
   * Depth of the last read XML tag
   */
  int actualDepth;

  /**
   * Name of the last read XML tag
   */
  TagName actualTagName;

  /**
   * Attributes and their values of the last read XML tag
   * Stored in a special object designed for this purpose
   */
  TagAttributes actualTagAttributes;

  /**
   * Stack for storing the XML tag hierarchy but only for the last read XML tag
   */
  List<TagName> parentTagNames;

  // Stack for keeping track if we are inside an annotation tag
  List<TagName> inside_annotation;
  
  bool lastWasListEnd;

  static bool suspended;

  /**
   * Used to register errors during the parsing phase
   * After this it is possible to check it
   * and if errors occur possible to stop converter
   */
  static unsigned int num_errors;
  static unsigned int num_warnings;

  /**
   *  Callback functions for LibXML SAX parser
   */
  void startelementHandler(const xmlChar * localname, const int nb_namespaces, const xmlChar ** namespaces, int nb_attributes, const xmlChar ** attributes);
  void endelementHandler(const xmlChar * localname);
  void characterdataHandler(const xmlChar * text, const int length);
  void commentHandler(const xmlChar * text);
  void startelementHandlerWhenXSDRead(const xmlChar * localname, const int nb_namespaces, const xmlChar ** namespaces, int nb_attributes, const xmlChar ** attributes);

  /** Callbacks cannot be member functions, use these static members as wrappers */
  static void wrapper_to_call_startelement_h(XMLParser *self, const xmlChar * localname, const xmlChar * prefix, const xmlChar * URI, int nb_namespaces, const xmlChar ** namespaces, int nb_attributes, int nb_defaulted, const xmlChar ** attributes);
  static void wrapper_to_call_endelement_h(XMLParser *self, const xmlChar * localname, const xmlChar * prefix, const xmlChar * URI);
  static void wrapper_to_call_characterdata_h(XMLParser *self, const xmlChar * ch, int len);
  static void wrapper_to_call_comment_h(XMLParser *self, const xmlChar * value);
  
  /** Callback for start element handler when we read the namespace of the xsd files only, */
  static void wrapper_to_call_startelement_when_xsd_read_h(XMLParser *self, const xmlChar * localname, const xmlChar * prefix, const xmlChar * URI, int nb_namespaces, const xmlChar ** namespaces, int nb_attributes, int nb_defaulted, const xmlChar ** attributes);

  static void warningHandler(void * ctx, const char * msg, ...);
  static void errorHandler(void * ctx, const char * msg, ...);

  /**
   * Converts name of read tag to enumerated value
   * and load it to actualTagName member
   *
   * mode argument indicates that it is called when
   * startelement or endelement arrived
   */
  void fillUpActualTagName(const char * localname, const tagMode mode);

  /**
   * Converts name and value of attributes of read tag
   * and fill actualTagAttributes object with current values
   */
  void fillUpActualTagAttributes(const char ** attributes, const int att_count);

  XMLParser(const XMLParser &); // not implemented
  XMLParser & operator=(const XMLParser &); // not implemented
public:
  XMLParser(const char * a_filename);
  ~XMLParser();

  /**
   * After an XMLParser object is born
   * there is need to connect it with a TTCN3Module object
   * for loading the read data into it
   */
  void connectWithModule(TTCN3Module * a_module);

  /**
   * Start syntax checking, validation and parse
   */
  void checkSyntax();
  void validate();
  void startConversion(TTCN3Module * a_module);

  static unsigned int getNumErrors() {
    return num_errors;
  }

  static unsigned int getNumWarnings() {
    return num_warnings;
  }

  static void incrNumErrors() {
    ++num_errors;
  }

  static void incrNumWarnings() {
    ++num_warnings;
  }

  const Mstring & getFilename() const {
    return filename;
  }

  int getActualLineNumber() const {
    return xmlSAX2GetLineNumber(context);
  }

  int getActualDepth() const {
    return actualDepth;
  }

  TagName getActualTagName() const {
    return actualTagName;
  }

  TagName getParentTagName() const {
    return parentTagNames.empty() ? n_NOTSET : parentTagNames.back();
  }

  const TagAttributes & getActualTagAttributes() const {
    return actualTagAttributes;
  }
};

#endif /* PARSER_HH_ */
