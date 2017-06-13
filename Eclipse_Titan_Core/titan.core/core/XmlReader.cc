/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Raduly, Csaba
 *
 ******************************************************************************/
/*
 * XmlReader.cc
 *
 *  Created on: 19-Nov-2008
 *      Author: ecsardu
 */

#include <libxml/xmlreader.h>
#include "XmlReader.hh"
#include "static_check.h"

#include "Encdec.hh"

XmlReaderWrap::XmlReaderWrap(TTCN_Buffer& buf)
: my_reader(0)
{
  // compile-time safety checks
  ENSURE_EQUAL(     XML_PARSER_SEVERITY_VALIDITY_WARNING, \
    xmlreader_lite::XML_PARSER_SEVERITY_VALIDITY_WARNING);
  ENSURE_EQUAL(     XML_PARSER_SEVERITY_VALIDITY_ERROR  , \
    xmlreader_lite::XML_PARSER_SEVERITY_VALIDITY_ERROR);
  ENSURE_EQUAL(     XML_PARSER_SEVERITY_WARNING, \
    xmlreader_lite::XML_PARSER_SEVERITY_WARNING);
  ENSURE_EQUAL(     XML_PARSER_SEVERITY_ERROR  , \
    xmlreader_lite::XML_PARSER_SEVERITY_ERROR);

  ENSURE_EQUAL(     XML_READER_TYPE_NONE, \
    xmlreader_lite::XML_READER_TYPE_NONE);
  ENSURE_EQUAL(     XML_READER_TYPE_ELEMENT, \
    xmlreader_lite::XML_READER_TYPE_ELEMENT);
  ENSURE_EQUAL(     XML_READER_TYPE_ATTRIBUTE, \
    xmlreader_lite::XML_READER_TYPE_ATTRIBUTE);
  ENSURE_EQUAL(     XML_READER_TYPE_TEXT, \
    xmlreader_lite::XML_READER_TYPE_TEXT);
  ENSURE_EQUAL(     XML_READER_TYPE_CDATA, \
    xmlreader_lite::XML_READER_TYPE_CDATA);
  ENSURE_EQUAL(     XML_READER_TYPE_ENTITY_REFERENCE, \
    xmlreader_lite::XML_READER_TYPE_ENTITY_REFERENCE);
  ENSURE_EQUAL(     XML_READER_TYPE_ENTITY, \
    xmlreader_lite::XML_READER_TYPE_ENTITY);
  ENSURE_EQUAL(     XML_READER_TYPE_PROCESSING_INSTRUCTION, \
    xmlreader_lite::XML_READER_TYPE_PROCESSING_INSTRUCTION);
  ENSURE_EQUAL(     XML_READER_TYPE_COMMENT, \
    xmlreader_lite::XML_READER_TYPE_COMMENT);
  ENSURE_EQUAL(     XML_READER_TYPE_DOCUMENT, \
    xmlreader_lite::XML_READER_TYPE_DOCUMENT);
  ENSURE_EQUAL(     XML_READER_TYPE_DOCUMENT_TYPE, \
    xmlreader_lite::XML_READER_TYPE_DOCUMENT_TYPE);
  ENSURE_EQUAL(     XML_READER_TYPE_DOCUMENT_FRAGMENT, \
    xmlreader_lite::XML_READER_TYPE_DOCUMENT_FRAGMENT);
  ENSURE_EQUAL(     XML_READER_TYPE_NOTATION, \
    xmlreader_lite::XML_READER_TYPE_NOTATION);
  ENSURE_EQUAL(     XML_READER_TYPE_WHITESPACE, \
    xmlreader_lite::XML_READER_TYPE_WHITESPACE);
  ENSURE_EQUAL(     XML_READER_TYPE_SIGNIFICANT_WHITESPACE, \
    xmlreader_lite::XML_READER_TYPE_SIGNIFICANT_WHITESPACE);
  ENSURE_EQUAL(     XML_READER_TYPE_END_ELEMENT, \
    xmlreader_lite::XML_READER_TYPE_END_ELEMENT);
  ENSURE_EQUAL(     XML_READER_TYPE_END_ENTITY, \
    xmlreader_lite::XML_READER_TYPE_END_ENTITY);
  ENSURE_EQUAL(     XML_READER_TYPE_XML_DECLARATION, \
    xmlreader_lite::XML_READER_TYPE_XML_DECLARATION);

  LIBXML_TEST_VERSION;
  if (0 == buf.get_len()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INCOMPL_MSG,
      "Cannot decode empty XML");
  }
  else {
    const char * encoding = 0;
    my_reader = xmlReaderForMemory((const char*)buf.get_data(), buf.get_len(),
      "uri:geller", encoding, 0);
    if (0 == my_reader) {
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG,
        "Failed to create XML reader"); // no further information available
    }
    else xmlTextReaderSetErrorHandler(my_reader, errorhandler, this);
  }
}

void
XmlReaderWrap::errorhandler(void * arg, const char * msg,
  xmlParserSeverities severity, xmlTextReaderLocatorPtr locator)
{
  //XmlReaderWrap *self = (XmlReaderWrap*)arg;
  (void)arg;
  (void)severity;
  TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNDEF, // TODO create a new ET_
    "XML error: %s at line %d", msg, xmlTextReaderLocatorLineNumber(locator));
}

XmlReaderWrap::~XmlReaderWrap()
{
  xmlFreeTextReader(my_reader);
  my_reader = 0;
}


#undef Read
int XmlReaderWrap::Read()
{
  return last_status = xmlTextReaderRead(my_reader); // assign and return
}

// Debug version of Read(). Both are present to support linking
// non-debug clients to the debug library.
int XmlReaderWrap::ReadDbg(const char *where)
{
#ifdef NDEBUG
  (void)where;
#else
  static boolean d = !!getenv("DEBUG_XMLREADER");
  if(d){
    last_status = xmlTextReaderRead(my_reader);
    if (last_status==1) {
      int type = NodeType();
      const char * name = (const char * )Name();
      const char * uri  = (const char * )NamespaceUri();
      int empty = IsEmptyElement();
      char closer = type == XML_READER_TYPE_DOCUMENT_TYPE ? '[' : (empty ? '/' : ' ');
      printf("X%dX type=%2d, @%2d <%c{%s}%s%c>",
        xmlTextReaderReadState(my_reader), type, xmlTextReaderDepth(my_reader),
        ((type!=XML_READER_TYPE_END_ELEMENT) ? ' ' : '/'),
        uri ? uri : "", name, closer);
    }
    else if (last_status==0) {
      fputs("XXX eof  ", stdout);
    }
    else {
      fputs("XXX error", stdout);
    }
    printf("\t%s\n", where);
    fflush(stdout);
    return last_status;
  } else
#endif
  {
    return last_status = xmlTextReaderRead(my_reader); // assign and return
  }
}

#if 0
xmlChar * XmlReaderWrap::ReadInnerXml()
{ return xmlTextReaderReadInnerXml(my_reader); }
#endif

xmlChar * XmlReaderWrap::ReadOuterXml() // used by Charstring.cc
{ return xmlTextReaderReadOuterXml(my_reader); }

xmlChar * XmlReaderWrap::ReadString() // used by Objid.cc
{ return xmlTextReaderReadString(my_reader); }

int XmlReaderWrap::Depth() // used everywhere
{ return xmlTextReaderDepth(my_reader); }

#if 0
int XmlReaderWrap::ReadAttributeValue()
{ return xmlTextReaderReadAttributeValue(my_reader); }

int XmlReaderWrap::AttributeCount()
{ return xmlTextReaderAttributeCount(my_reader); }

int XmlReaderWrap::HasAttributes()
{ return xmlTextReaderHasAttributes(my_reader); }

int XmlReaderWrap::HasValue()
{ return xmlTextReaderHasValue(my_reader); }

int XmlReaderWrap::IsDefault()
{ return xmlTextReaderIsDefault(my_reader); }
#endif

int XmlReaderWrap::IsEmptyElement() // used often
{ return xmlTextReaderIsEmptyElement(my_reader); }

int XmlReaderWrap::NodeType()
{ return xmlTextReaderNodeType(my_reader); }

#if 0
int XmlReaderWrap::QuoteChar()
{ return xmlTextReaderQuoteChar(my_reader); }

int XmlReaderWrap::ReadState()
{ return xmlTextReaderReadState(my_reader); }
#endif

int XmlReaderWrap::IsNamespaceDecl() // used while processing attributes
{ return xmlTextReaderIsNamespaceDecl(my_reader); }

#if 0
const xmlChar * XmlReaderWrap::BaseUri()
{ return xmlTextReaderConstBaseUri(my_reader); }
#endif

const xmlChar * XmlReaderWrap::LocalName()
{ return xmlTextReaderConstLocalName(my_reader); }

const xmlChar * XmlReaderWrap::Name()
{ return xmlTextReaderConstName(my_reader); }

const xmlChar * XmlReaderWrap::NamespaceUri()
{ return xmlTextReaderConstNamespaceUri(my_reader); }

const xmlChar * XmlReaderWrap::Prefix()
{ return xmlTextReaderConstPrefix(my_reader); }

#if 0
const xmlChar * XmlReaderWrap::XmlLang()
{ return xmlTextReaderConstXmlLang(my_reader); }
#endif

const xmlChar * XmlReaderWrap::Value()
{ return xmlTextReaderConstValue(my_reader); }

xmlChar * XmlReaderWrap::NewValue()
{ return xmlTextReaderValue(my_reader); }

xmlChar * XmlReaderWrap::LookupNamespace( const xmlChar *prefix)
{ return xmlTextReaderLookupNamespace(my_reader,prefix); }

#if 0
int XmlReaderWrap::Close()
{ return xmlTextReaderClose(my_reader); }

xmlChar * XmlReaderWrap::GetAttributeNo( int no)
{ return xmlTextReaderGetAttributeNo(my_reader,no); }

xmlChar * XmlReaderWrap::GetAttribute( const xmlChar *name)
{ return xmlTextReaderGetAttribute(my_reader,name); }

xmlChar * XmlReaderWrap::GetAttributeNs( const xmlChar *localName, const xmlChar *namespaceURI)
{ return xmlTextReaderGetAttributeNs(my_reader,localName,namespaceURI); }

int XmlReaderWrap::MoveToAttributeNo( int no)
{ return xmlTextReaderMoveToAttributeNo(my_reader,no); }
#endif

int XmlReaderWrap::MoveToAttribute( const xmlChar *name)
{ return xmlTextReaderMoveToAttribute(my_reader,name); }

//int XmlReaderWrap::MoveToAttributeNs( const xmlChar *localName, const xmlChar *namespaceURI)
//{ return xmlTextReaderMoveToAttributeNs(my_reader,localName,namespaceURI); }

#undef AdvanceAttribute
int XmlReaderWrap::AdvanceAttribute()
{
  int rez;
  for (rez = MoveToNextAttribute(); rez==1; rez = MoveToNextAttribute()) {
    if (!xmlTextReaderIsNamespaceDecl(my_reader)) break;
  }
  if (rez != 0) // success(1) or failure (-1)
    return rez;
  // 0 means no more attributes. Back to the element.
  rez = xmlTextReaderMoveToElement(my_reader);
  return -(rez == -1); // if -1, return -1 else return 0
}

int XmlReaderWrap::AdvanceAttributeDbg(const char *where)
{
  int rez;
  for (rez = MoveToNextAttributeDbg(where); rez==1; rez = MoveToNextAttributeDbg(where)) {
    if (!xmlTextReaderIsNamespaceDecl(my_reader)) break;
  }
  if (rez != 0) // success(1) or failure (-1)
    return rez;
  // 0 means no more attributes. Back to the element.
  rez = xmlTextReaderMoveToElement(my_reader);
  return -(rez == -1); // if -1, return -1 else return 0
}


#undef MoveToFirstAttribute
int XmlReaderWrap::MoveToFirstAttribute()
{ return xmlTextReaderMoveToFirstAttribute(my_reader); }

int XmlReaderWrap::MoveToFirstAttributeDbg(const char *where)
{
  int ret = xmlTextReaderMoveToFirstAttribute(my_reader);
#ifdef NDEBUG
  (void)where;
#else
  static boolean d = !!getenv("DEBUG_XMLREADER");
  if (d) {
    switch (ret) {
    case 1: {//OK
      const xmlChar * name = xmlTextReaderConstLocalName(my_reader);
      const xmlChar * val  = xmlTextReaderConstValue(my_reader);
      printf("#%dX %s='%s'", xmlTextReaderReadState(my_reader), name, val);
      break;}
    case 0: {// not found
      const xmlChar * name = xmlTextReaderConstLocalName(my_reader);
      printf("#%dX no attribute found in <%s>", xmlTextReaderReadState(my_reader), name);
      break;}
    default:
      break;
    }

    printf("\t%s\n", where);
    fflush(stdout);
  }
  //return xmlTextReaderMoveToFirstAttribute(my_reader);
#endif
  return ret;
}

#undef MoveToNextAttribute
int XmlReaderWrap::MoveToNextAttribute()
{ return xmlTextReaderMoveToNextAttribute(my_reader); }

int XmlReaderWrap::MoveToNextAttributeDbg(const char *where)
{
  int ret = xmlTextReaderMoveToNextAttribute(my_reader);
#ifdef NDEBUG
  (void)where;
#else
  static boolean d = !!getenv("DEBUG_XMLREADER");
  if (d) {
    switch (ret) {
    case 1: {//OK
      const xmlChar * name = xmlTextReaderConstLocalName(my_reader);
      const xmlChar * val  = xmlTextReaderConstValue(my_reader);
      printf("X%dX %s='%s'", xmlTextReaderReadState(my_reader), name, val);
      break;}
    case 0: {// not found
      const xmlChar * name = xmlTextReaderConstLocalName(my_reader);
      printf("X%dX no more attributes found after '%s'", xmlTextReaderReadState(my_reader), name);
      break;}
    default:
      break;
    }

    printf("\t%s\n", where);
    fflush(stdout);
  }
  //return xmlTextReaderMoveToNextAttribute(my_reader);
#endif
  return ret;
}

#undef MoveToElement
int XmlReaderWrap::MoveToElement()
{ return xmlTextReaderMoveToElement(my_reader); }

int XmlReaderWrap::MoveToElementDbg(const char *where)
{
  int ret = xmlTextReaderMoveToElement(my_reader);
#ifdef NDEBUG
  (void)where;
#else
  static boolean d = !!getenv("DEBUG_XMLREADER");
  if (d) {
    const xmlChar * name = xmlTextReaderConstLocalName(my_reader);
    const xmlChar * val  = xmlTextReaderConstValue(my_reader);
    printf("X%dX <%s='%s'>\n", xmlTextReaderReadState(my_reader), name, val);
  }

  printf("\t%s\n", where);
  fflush(stdout);
#endif
  return ret;
}

//int XmlReaderWrap::GetParserLineNumber()
//{ return xmlTextReaderGetParserLineNumber(my_reader); }

//int XmlReaderWrap::GetParserColumnNumber()
//{ return xmlTextReaderGetParserColumnNumber(my_reader); }

#if 0
int XmlReaderWrap::Next()
{ return xmlTextReaderNext(my_reader); }

int XmlReaderWrap::NextSibling()
{ return xmlTextReaderNextSibling(my_reader); }

int XmlReaderWrap::Normalization()
{ return xmlTextReaderNormalization(my_reader); }

int XmlReaderWrap::SetParserProp( int prop, int value)
{ return xmlTextReaderSetParserProp(my_reader,prop,value); }

int XmlReaderWrap::GetParserProp( int prop)
{ return xmlTextReaderGetParserProp(my_reader,prop); }

xmlParserInputBufferPtr XmlReaderWrap::GetRemainder()
{ return xmlTextReaderGetRemainder(my_reader); }

xmlNodePtr XmlReaderWrap::CurrentNode()
{ return xmlTextReaderCurrentNode(my_reader); }

xmlNodePtr XmlReaderWrap::Preserve()
{ return xmlTextReaderPreserve(my_reader); }

int XmlReaderWrap::PreservePattern( const xmlChar *pattern, const xmlChar **namespaces)
{ return xmlTextReaderPreservePattern(my_reader,pattern,namespaces); }

xmlDocPtr XmlReaderWrap::CurrentDoc()
{ return xmlTextReaderCurrentDoc(my_reader); }

xmlNodePtr XmlReaderWrap::Expand()
{ return xmlTextReaderExpand(my_reader); }

int XmlReaderWrap::IsValid()
{ return xmlTextReaderIsValid(my_reader); }

int XmlReaderWrap::RelaxNGValidate( const char *rng)
{ return xmlTextReaderRelaxNGValidate(my_reader,rng); }

int XmlReaderWrap::RelaxNGSetSchema( xmlRelaxNGPtr schema)
{ return xmlTextReaderRelaxNGSetSchema(my_reader,schema); }

int XmlReaderWrap::SchemaValidate( const char *xsd)
{ return xmlTextReaderSchemaValidate(my_reader,xsd); }

int XmlReaderWrap::SchemaValidateCtxt( xmlSchemaValidCtxtPtr ctxt, int options)
{ return xmlTextReaderSchemaValidateCtxt(my_reader,ctxt,options); }

int XmlReaderWrap::SetSchema( xmlSchemaPtr schema)
{ return xmlTextReaderSetSchema(my_reader,schema); }

int XmlReaderWrap::Standalone()
{ return xmlTextReaderStandalone(my_reader); }
#endif

long XmlReaderWrap::ByteConsumed()
{ return xmlTextReaderByteConsumed(my_reader); }

//int XmlReaderWrap::LocatorLineNumber(xmlTextReaderLocatorPtr locator)
//{ return xmlTextReaderLocatorLineNumber(locator); }

//xmlChar * XmlReaderWrap::LocatorBaseURI(xmlTextReaderLocatorPtr locator)
//{ return xmlTextReaderLocatorBaseURI(locator); }

#if 0
void XmlReaderWrap::SetErrorHandler( xmlTextReaderErrorFunc f, void *arg)
{ return xmlTextReaderSetErrorHandler(my_reader,f,arg); }

void XmlReaderWrap::SetStructuredErrorHandler( xmlStructuredErrorFunc f, void *arg)
{ return xmlTextReaderSetStructuredErrorHandler(my_reader,f,arg); }

void XmlReaderWrap::GetErrorHandler( xmlTextReaderErrorFunc *f, void **arg)
{ return xmlTextReaderGetErrorHandler(my_reader,f,arg); }
#endif

#ifndef NDEBUG
void XmlReaderWrap::Status()
{
  //const xmlChar *string   = xmlTextReaderConstString    (my_reader);
  const xmlChar *baseuri  = xmlTextReaderConstBaseUri   (my_reader);
  const xmlChar *localname= xmlTextReaderConstLocalName (my_reader);
  const xmlChar *name     = xmlTextReaderConstName      (my_reader);
  const xmlChar *ns_uri   = xmlTextReaderConstNamespaceUri(my_reader);
  const xmlChar *prefix   = xmlTextReaderConstPrefix    (my_reader);
  const xmlChar *lang     = xmlTextReaderConstXmlLang   (my_reader);
  const xmlChar *value    = xmlTextReaderConstValue     (my_reader);
  const xmlChar *encoding = xmlTextReaderConstEncoding  (my_reader);
  const xmlChar *version  = xmlTextReaderConstXmlVersion(my_reader);

  printf ("XML reader %d deep:\n"
    "\tbaseUri   = '%s'\n"
    "\tprefix    = '%s'\n"
    "\tlocalname = '%s'\n"
    "\tname      = '%s%s'\n"
    "\tns-Uri    = '%s'\n"
    "\txml lang  = '%s'\n"
    "\tvalue     = '%s'\n"
    "\tencoding  = '%s'\n"
    "\txml ver   = '%s'\n"
    , xmlTextReaderDepth(my_reader)
    , baseuri
    , prefix
    , localname
    , name, (xmlTextReaderIsEmptyElement(my_reader) ? "/" : "")
    , ns_uri
    , lang
    , value
    , encoding
    , version);
}
#endif



