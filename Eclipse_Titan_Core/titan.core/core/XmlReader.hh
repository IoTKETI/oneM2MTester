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
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
#ifndef XML_READER_HH_
#define XML_READER_HH_

#ifdef __XML_XMLREADER_H__
// The real xmlreader.h is included; hide our "lite" version in a namespace.
namespace xmlreader_lite {
#endif

// "lite" version of xmlreader.h

typedef void * xmlTextReaderPtr;
typedef void * xmlTextReaderLocatorPtr;
typedef unsigned char xmlChar;
typedef enum {
  XML_PARSER_SEVERITY_VALIDITY_WARNING = 1,
  XML_PARSER_SEVERITY_VALIDITY_ERROR = 2,
  XML_PARSER_SEVERITY_WARNING = 3,
  XML_PARSER_SEVERITY_ERROR = 4
} xmlParserSeverities;

typedef enum {
  XML_READER_TYPE_NONE = 0,
  XML_READER_TYPE_ELEMENT = 1,
  XML_READER_TYPE_ATTRIBUTE = 2,
  XML_READER_TYPE_TEXT = 3,
  XML_READER_TYPE_CDATA = 4,
  XML_READER_TYPE_ENTITY_REFERENCE = 5,
  XML_READER_TYPE_ENTITY = 6,
  XML_READER_TYPE_PROCESSING_INSTRUCTION = 7,
  XML_READER_TYPE_COMMENT = 8,
  XML_READER_TYPE_DOCUMENT = 9,
  XML_READER_TYPE_DOCUMENT_TYPE = 10,
  XML_READER_TYPE_DOCUMENT_FRAGMENT = 11,
  XML_READER_TYPE_NOTATION = 12,
  XML_READER_TYPE_WHITESPACE = 13,
  XML_READER_TYPE_SIGNIFICANT_WHITESPACE = 14,
  XML_READER_TYPE_END_ELEMENT = 15,
  XML_READER_TYPE_END_ENTITY = 16,
  XML_READER_TYPE_XML_DECLARATION = 17
} xmlReaderTypes;

typedef void (*xmlFreeFunc)(void *mem);
extern "C" xmlFreeFunc xmlFree;

#ifdef __XML_XMLREADER_H__
} // end namespace
#endif

class TTCN_Buffer;

/** Wrapper for xmlTextReader... methods */
class XmlReaderWrap {
  xmlTextReaderPtr my_reader; ///< the my_reader instance
  int last_status; ///< the success code of the last read operation
  static void errorhandler(void * arg, const char * msg,
    xmlParserSeverities severity, xmlTextReaderLocatorPtr locator);
  /// Copy constructor disabled
  XmlReaderWrap(const XmlReaderWrap&);
  /// Assignment disabled
  XmlReaderWrap& operator=(const XmlReaderWrap&);
public:
  XmlReaderWrap(TTCN_Buffer& buf);
  ~XmlReaderWrap();

  //int Setup( xmlParserInputBufferPtr input, const char *URL, const char *encoding, int options)
  //{ return xmlTextReaderSetup(my_reader,input,URL,encoding,options); }

  /** Move the read position to the next node.
   *  @return 1 on success, 0 if no more nodes to read, -1 on error */
  int Read();
  int ReadDbg(const char *where);
  
#if 0
  /** Return the child nodes of the current node.
   *
   * This does not include the element itself, only its children.
   *
   * @return a newly allocated string. The caller must deallocate the string
   * using \c xmlFree.
   */
  xmlChar * ReadInnerXml();
#endif

  /** Return the contents of the current node, including child nodes and markup.
   *
   * This includes the element itself.
   *
   * @return a newly allocated string. The caller must deallocate the string
   * using \c xmlFree.
   */
  xmlChar * ReadOuterXml();

  /** Reads the contents of an element or a text node as a string.
   *
   * @return a newly allocated string containing the contents of the Element
   * or Text node, or NULL if the my_reader is positioned on any other type
   * of node. The caller must deallocate the string using \c xmlFree.
   */
  xmlChar * ReadString();
#if 0
  int ReadAttributeValue();

  int AttributeCount();

  int HasAttributes();

  int HasValue();

  int IsDefault();
#endif
  int Depth();

  int IsEmptyElement();

  int NodeType();
#if 0
  int QuoteChar();

  int ReadState();
#endif
  inline int Ok()
  { return last_status > 0; }

  int IsNamespaceDecl();

  /** @name Properties
   *
   * @{
   * These return constant strings; the caller must not modify or free them */
#if 0
  const xmlChar * BaseUri();
#endif

  /// Return the unqualified name of the node. It is "#text" for text nodes.
  const xmlChar * LocalName();

  /// Return the qualified name of the node (Prefix + ':' + LocalName)
  const xmlChar * Name();

  /// Return the URI defining the namespace associated with the node
  const xmlChar * NamespaceUri();

  /// Return the namespace prefix associated with the node
  const xmlChar * Prefix();
#if 0
  const xmlChar * XmlLang();
#endif
  /// Return the text value of the node, if present
  const xmlChar * Value();

  /** Returns a newly allocated string containing Value.
   * The caller must deallocate the string with xmlFree() */
  xmlChar * NewValue();
  /** @} */

  /** Resolves a namespace prefix in the scope of the current element
   *
   * @param prefix to resolve. Use NULL for the default namespace.
   * @return a newly allocated string containing the namespace URI.
   *  It must be deallocated by the caller. */
  xmlChar * LookupNamespace( const xmlChar *prefix);

#if 0
  int Close();

  xmlChar * GetAttributeNo( int no);

  xmlChar * GetAttribute( const xmlChar *name);

  xmlChar * GetAttributeNs( const xmlChar *localName, const xmlChar *namespaceURI);

  /** Moves the read position to the attribute with the specified index.
   *
   * @param no zero-based index of the attribute
   * @return 1 for success, -1 for error, 0 if not found
   */
  int MoveToAttributeNo( int no);
#endif

  /** Moves the read position to the attribute with the specified name.
   *
   * @param name: the qualified name of the attribute
   * @return 1 for success, -1 for error, 0 if not found
   */
  int MoveToAttribute(const xmlChar *name);

  /** Moves the read position to the attribute with the specified name.
   *
   * @param localName local name of the attribute
   * @param namespaceURI namespace URI of the attribute
   * @return 1 for success, -1 for error, 0 if not found
   */
  int MoveToAttributeNs(const xmlChar *localName, const xmlChar *namespaceURI);

  /** Moves the read position to the first attribute of the current node.
   *
   * @return 1 for success, -1 for error, 0 if not found
   */
  int MoveToFirstAttribute();
  int MoveToFirstAttributeDbg(const char *where);

  /** Moves the read position to the next attribute of the current node.
   *
   * @return 1 for success, -1 for error, 0 if not found
   */
  int MoveToNextAttribute();
  int MoveToNextAttributeDbg(const char *where);

  /** Moves the read position to the node that contains the current Attribute node.
   *
   * @return 1 for success, -1 for error, 0 if not moved
   */
  int MoveToElement();
  int MoveToElementDbg(const char *where);

  /** Utility function for navigating attributes.
   * Calls MoveToNextAttribute(). If there are no more attributes,
   * it calls MoveToElement() to stop iterating over attributes.
   *
   * @return 1 for success, -1 for error, 0 if no more attributes
   * (MoveToElement was called and didn't fail)  */
  int AdvanceAttribute();
  int AdvanceAttributeDbg(const char *where);

#if 0
  int GetParserLineNumber();

  int GetParserColumnNumber();

  int Next();

  int NextSibling();

  int Normalization();

  int SetParserProp( int prop, int value);

  int GetParserProp( int prop);

  xmlParserInputBufferPtr GetRemainder();

  xmlNodePtr CurrentNode();

  xmlNodePtr Preserve();

  int PreservePattern( const xmlChar *pattern, const xmlChar **namespaces);

  xmlDocPtr CurrentDoc();

  xmlNodePtr Expand();

  int IsValid();

  int RelaxNGValidate( const char *rng);

  int RelaxNGSetSchema( xmlRelaxNGPtr schema);

  int SchemaValidate( const char *xsd);

  int SchemaValidateCtxt( xmlSchemaValidCtxtPtr ctxt, int options);

  int SetSchema( xmlSchemaPtr schema);
#endif
  int Standalone();

  long ByteConsumed();

  int LocatorLineNumber(xmlTextReaderLocatorPtr locator);

  xmlChar * LocatorBaseURI(xmlTextReaderLocatorPtr locator);

#if 0
  void SetErrorHandler( xmlTextReaderErrorFunc f, void *arg);

  void SetStructuredErrorHandler( xmlStructuredErrorFunc f, void *arg);

  void GetErrorHandler( xmlTextReaderErrorFunc *f, void **arg);
#endif

#if defined(__GNUC__) && !defined(NDEBUG)
#define Read() ReadDbg(__PRETTY_FUNCTION__)
#define MoveToFirstAttribute() MoveToFirstAttributeDbg(__PRETTY_FUNCTION__)
#define MoveToNextAttribute()  MoveToNextAttributeDbg(__PRETTY_FUNCTION__)
#define AdvanceAttribute()     AdvanceAttributeDbg(__PRETTY_FUNCTION__)
#endif

#ifndef NDEBUG
  void Status();
#endif

};

#endif
