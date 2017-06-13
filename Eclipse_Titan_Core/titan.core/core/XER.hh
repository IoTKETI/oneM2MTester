/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Baranyi, Botond
 *   Raduly, Csaba
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
#ifndef XER_HH_
#define XER_HH_

#include "Types.h"
#include "Encdec.hh"
#include <stddef.h> // for size_t
#include <string.h> // strncmp for the inline function

class XmlReaderWrap;

class Base_Type;
#ifdef TITAN_RUNTIME_2
class Record_Of_Type;
struct Erroneous_descriptor_t;
#else
namespace PreGenRecordOf {
  class PREGEN__RECORD__OF__UNIVERSAL__CHARSTRING;
  class PREGEN__RECORD__OF__UNIVERSAL__CHARSTRING__OPTIMIZED;
}
#endif
class TTCN_Module;

/** @defgroup XER XER codec
 * @{
 *
 * @brief ASN.1 XML Encoding Rules, ITU-T Rec X.693 and amd1
 */

/** XER flags for various uses.
 *
 * Low values specify the XML encoding variant (Basic, Canonical, Extended)
 * Other bits have dual uses:
 * - set in XERdescriptor_t::xer_bits, according to XML encoding attributes
 * - passed in as additional flags in the \c flavor parameter, usually
 * to XER_encode. These are used when encoding attributes in a parent type
 * influence the encoding of its components (e.g. EMBED-VALUES on a record
 * change the encoding of all components).
 */
enum XER_flavor {
  XER_NONE            = 0,
  XER_BASIC           = 1U << 0, /**< Basic XER with indentation */
  XER_CANONICAL       = 1U << 1, /**< Canonical XER, no indentation */
  XER_EXTENDED        = 1U << 2, /**< Extended XER */
  DEF_NS_PRESENT      = 1U << 3, // 0x08
  DEF_NS_SQUASHED     = 1U << 4, // 0x10

  /* Additional flags, for the parent to pass information to its children
   * (when the parent affects the child, e.g. LIST) */
  XER_ESCAPE_ENTITIES = 1U << 5, /**< Escape according to X.680/2002, 11.15.8,
  used internally by UNIVERSAL_CHARSTRING. */
  XER_RECOF          = 1U << 6, /**< Generating code for the contained type
  of a record-of/set-of. Only affects BOOLEAN, CHOICE, ENUMERATED and NULL
  (see Table 5 in X.680 (11/2008) clause 26.5) */

  /* More flags for XERdescriptor_t::xer_bits */
  ANY_ATTRIBUTES = 1U << 7,  //  0xooo80
  ANY_ELEMENT    = 1U << 8,  //  0xoo100
  XER_ATTRIBUTE  = 1U << 9,  //  0xoo200
  BASE_64        = 1U << 10, //  0xoo400
  XER_DECIMAL    = 1U << 11, //  0xoo800
  // DEFAULT-FOR-EMPTY has its own field
  EMBED_VALUES   = 1U << 12, //  0xo1000
  /** LIST encoding instruction for record-of/set-of. */
  XER_LIST       = 1U << 13, //  0xo2000
  // NAME is stored in the descriptor
  // NAMESPACE is folded into the name
  XER_TEXT       = 1U << 14, //  0xo4000
  UNTAGGED       = 1U << 15, //  0xo8000
  USE_NIL        = 1U << 16, //  0x10000
  USE_NUMBER     = 1U << 17, //  0x20000
  USE_ORDER      = 1U << 18, //  0x40000
  USE_QNAME      = 1U << 19, //  0x80000
  USE_TYPE_ATTR  = 1U << 20, // 0x100000, either USE-TYPE or USE-UNION
  HAS_1UNTAGGED  = 1U << 21, // 0x200000 member, and it's character-encodable
  // another hint to pass down to the children:
  PARENT_CLOSED  = 1U << 22, // 0x400000
  FORM_UNQUALIFIED=1U << 23, // 0X800000 (qualified is more frequent)
  XER_TOPLEVEL   = 1U << 24, //0X1000000 (toplevel, for decoding)
  SIMPLE_TYPE    = 1U << 25, /*0X2000000 always encode on one line:
  <foo>content</foo>, never <foo>\ncontent\n</foo> */
  BXER_EMPTY_ELEM= 1U << 26,  /*0X4000000 boolean and enum encode themselves
  as empty elements in BXER only. This also influences them in record-of */
  ANY_FROM       = 1U << 27, // 0x8000000 anyElement from ... or anyAttributes from ...
  ANY_EXCEPT     = 1U << 28, // 0x10000000 anyElement except ... or anyAttributes except ...
  EXIT_ON_ERROR  = 1U << 29, /* 0x20000000 clean up and exit instead of throwing
  a decoding error, used on alternatives of a union with USE-UNION */
  XER_OPTIONAL   = 1U << 30, // 0x40000000 is an optional field of a record or set
  BLOCKED        = 1U << 31,  // 0x80000000 either ABSTRACT or BLOCK
  
  /**< All the "real" XER flavors plus DEF_NS + XER_OPTIONAL*/
  XER_MASK            = 0x1FU | XER_OPTIONAL
};

enum XER_flavor2 {
    USE_NIL_PARENT_TAG = 1U << 0, // Content field has attribute that was read by parent
    FROM_UNION_USETYPE = 1U << 1, // When the parent of a useUnion field is a union with useType
    THIS_UNION = 1U << 2 // When the type is a union
};

/** WHITESPACE actions.
 * Note that WHITESPACE_COLLAPSE includes the effect of WHITESPACE_REPLACE
 * and the code relies on WHITESPACE_COLLAPSE having the higher value. */
enum XER_whitespace_action {
  WHITESPACE_PRESERVE,
  WHITESPACE_REPLACE,
  WHITESPACE_COLLAPSE
};

/**
 * XSD type variants. For example: XSD:integer XSD:binary etc.
 * A field can only have one of these variants.
 * This enum should be in sync with the XSD_types enum in XSD_Types.hh
 */
typedef enum {
  XSD_NONE = 0,
  XSD_ANYSIMPLETYPE,
  XSD_ANYTYPE,
  XSD_STRING,
  XSD_NORMALIZEDSTRING,
  XSD_TOKEN,
  XSD_NAME,
  XSD_NMTOKEN,
  XSD_NCName,
  XSD_ID,
  XSD_IDREF,
  XSD_ENTITY,
  XSD_HEXBINARY,
  XSD_BASE64BINARY,
  XSD_ANYURI,
  XSD_LANGUAGE,
  XSD_INTEGER,
  XSD_POSITIVEINTEGER,
  XSD_NONPOSITIVEINTEGER,
  XSD_NEGATIVEINTEGER,
  XSD_NONNEGATIVEINTEGER,
  XSD_LONG,
  XSD_UNSIGNEDLONG,
  XSD_INT,
  XSD_UNSIGNEDINT,
  XSD_SHORT,
  XSD_UNSIGNEDSHORT,
  XSD_BYTE,
  XSD_UNSIGNEDBYTE,
  XSD_DECIMAL,
  XSD_FLOAT,
  XSD_DOUBLE,
  XSD_DURATION,
  XSD_DATETIME,
  XSD_TIME,
  XSD_DATE,
  XSD_GYEARMONTH,
  XSD_GYEAR,
  XSD_GMONTHDAY,
  XSD_GDAY,
  XSD_GMONTH,
  XSD_NMTOKENS,
  XSD_IDREFS,
  XSD_ENTITIES,
  XSD_QNAME,
  XSD_BOOLEAN
} XSD_types;

/// Check that \p f has the canonical flavor.
inline boolean is_canonical(unsigned int f)
{
  return (f & XER_CANONICAL) != 0;
}

// exer 0 for Basic/Canonical XER, 1 for EXER
// TODO: It would be better to have an enum for the exers
inline boolean is_exer(unsigned int f)
{
  return (f & XER_EXTENDED) != 0;
}

/** Is this a member of a SEQUENCE OF
 *
 * @param f XER flavor
 * @return \c true if \p contains \c XER_RECOF, \c false otherwise
 */
inline boolean is_record_of(unsigned int f)
{
  return (f & XER_RECOF) != 0;
}

/** Do list encoding
 *
 * This is now hijacked to mean "the enclosing type told us to omit our tag".
 * Hence the check for USE-NIL too.
 *
 * @param f XER flavor
 * @return \c true if \c XER_EXTENDED and either \c XER_LIST or \c USE_NIL is set.
 */
inline boolean is_exerlist(unsigned int f)
{
  return (f & XER_EXTENDED) && ((f & (XER_LIST|USE_NIL|USE_TYPE_ATTR)) != 0);
}

/** Descriptor for XER encoding/decoding during runtime.
 *
 * This structure contains XER enc/dec information for the runtime.
 *
 * There is an instance of this struct for most TTCN3/ASN1 types.
 * Because TITAN generates type aliases (typedefs) when one type references
 * another (e.g. "type integer i1" results in "typedef INTEGER i1"),
 * this struct holds information to distinguish them during encoding.
 *
 * Only those encoding instructions need to be recorded which can apply to
 * scalar types (e.g. BOOLEAN, REAL, etc., usually implemented by classes in core/)
 * because the same code needs to handle all varieties.
 *
 *  - ANY-ELEMENT : UFT8String
 *  - BASE64      : OCTET STRING, open type, restricted character string
 *  - DECIMAL     : REAL
 *  - NAME        : anything (this is already present as \c name)
 *  - NAMESPACE   : hmm
 *  - TEXT        : INTEGER, enum
 *  - USE-NUMBER  : enum
 *  - WHITESPACE  : restricted character string
 *
 * ANY-ATTRIBUTE, EMBED-VALUES, LIST, USE-TYPE, USE-UNION apply to sequence/choice types;
 * their effect will be resolved by the compiler.
 *
 * Instances of this type are written by the compiler into the generated code,
 * one for each type. For a TTCN3 type foo_bar, there will be a class
 * foo__bar and a XERdescriptor_t instance named foo__bar_xer_.
 *
 * Each built-in type has a descriptor (e.g. INTEGER_xer_) in the runtime.
 *
 * The \a name field contains the closing tag including a newline, e.g.
 * \c "</INTEGER>\n". This allows for a more efficient output of the tags,
 * minimizing the number of one-character inserts into the buffer.
 *
 * The start tag is written as an 'open angle bracket' character,
 * followed by the \a name field without its first two characters (\c "</" ).
 *
 * In case of the canonical encoding (\c CXER ) there is no indenting,
 * so the final newline is omitted by reducing the length by one.
 *
 * Example:
 * @code
 * int Foo::XER_encode(const XERdescriptor_t& p_td,
 *                     TTCN_Buffer& p_buf, unsigned int flavor, int indent, embed_values_enc_struct_t*) const {
 *   int canon = is_canonical(flavor);
 *   if (!canon) do_indent(p_buf, indent);
 *   // output the start tag
 *   p_buf.put_c('<');
 *   p_buf.put_s((size_t)p_td.namelen-2-canon, (const unsigned char*)p_td.name+2);
 *   // this is not right if Foo has attributes :(
 *   // we'll need to reduce namelen further (or just get rid of this hackery altogether)
 *
 *   // output actual content
 *   p_buf.put_.....
 *
 *   // output the closing tag
 *   if (!canon) do_indent(p_buf, indent);
 *   p_buf.put_s((size_t)p_td.namelen-canon, (const unsigned char*)p_td.name);
 * }
 * @endcode
 *
 * Empty element tag:
 *
 * @code
 * int Foo::XER_encode(const XERdescriptor_t& p_td,
 *                     TTCN_Buffer& p_buf, unsigned int flavor, int indent, embed_values_enc_struct_t*) const {
 *   int canon = is_canonical(flavor);
 *   if (!canon) do_indent(p_buf, indent);
 *   // output an empty element tag
 *   p_buf.put_c('<');
 *   p_buf.put_s((size_t)p_td.namelen-4, (const unsigned char*)p_td.name+2);
 *   p_buf.put_s(3 - canon, (const unsigned char*)"/>\n");
 * }
 * @endcode
 *
 * @note We don't generate the XML prolog. This is required for Canonical XER
 * (X.693 9.1.1) and permitted for Basic-XER (8.2.1).
 *
 * @note X.693 amd1 (EXER) 10.3.5 states: If an "ExtendedXMLValue" is empty,
 * and its associated tags have not been removed by the use of an UNTAGGED
 * encoding instruction, then the associated preceding and following tags
 * <b>can (as an encoder's option)</b> be replaced with
 * an XML empty-element tag (see ITU-T Rec. X.680 | ISO/IEC 8824-1, 16.8).
 * This is called the associated empty-element tag.
 *
 * @note X.693 (XER) 9.1.4 states: (for Canonical XER)
 * If the XML value notation permits the use of an XML empty-element tag
 * (see ITU-T Rec. X.680 |ISO/IEC 8824-1, 15.5 and 16.8),
 * then this empty-element tag @b shall be used.
 *
 * @note After editing XERdescriptor_t, make sure to change XER_STRUCT2 here
 * and generate_code_xerdescriptor() in Type.cc.
 * */
struct XERdescriptor_t
{
  /** (closing) Tag name, including a newline.
   * First is for basic and canonical XER, second for EXER */
  const          char *names[2];
  /** Length of closing tag string (strlen of names[i]) */
  const unsigned short namelens[2];
  /** Various EXER flags
   * This field is used as a flag carrier integer.
   * Each bit (flag) in the integer has a name which comes from the XER_flavor enum.
   * The XER_flavor enum holds 32 elements (bits) which fits into this
   * unsigned int. If we need to use the flags form the XER_flavor2 enum then
   * a new xer_bits field needs to be created.
   * 
   * Some possible uses: 
   *  - in the xer descriptor of the types
   *  - in conditions: if (p_td.xer_bits & UNTAGGED) ...
   */
  const unsigned int xer_bits; // TODO which types what flags mean what ?
  /** Whitespace handling */
  const XER_whitespace_action whitespace;
  /** value to compare for DEFAULT-FOR-EMPTY */
  const Base_Type* dfeValue;
  /** The module to which the type belongs. May be NULL in a descriptor
   * for a built-in type, e.g. in INTEGER_xer_ */
  const TTCN_Module* my_module;
  /** Index into the module's namespace list.
   * -1 means no namespace.
   * >=+0 and FORM_UNQUALIFIED means that there IS a namespace,
   * it just doesn't show up in the XML (but libxml2 will return it). */
  const int ns_index;
  
  /** Number of namespace URIs*/
  const unsigned short nof_ns_uris; 
  
  /** List of namespace URIs
    * In case of "anyElement" variants this list contains the valid ("anyElement from ...")
    * or invalid ("anyElement except ...") namespace URIs. 
    * The unqualified namespace is marked by an empty string ("").*/
  const char** ns_uris; 
  
  /** Points to the element type's XER descriptor in case of 'record of' and 'set of' types */
  const XERdescriptor_t* oftype_descr;
  
  /** Fraction digits value
    * It is already checked that is must not be a negative number.
    * The -1 value is used to determine if fractionDigits encoding instruction is present,
    * so if the value is -1, no checks will be made. */
  const int fractionDigits;
  
  /** XSD type of field */
  const XSD_types xsd_type;
};

/** Information related to the embedded values in XML encoding
  * 
  * Used when a record/set with the EMBED-VALUES coding instruction contains
  * one or more record of/set of fields. */
struct embed_values_enc_struct_t
{
#ifdef TITAN_RUNTIME_2
  /** Stores the array of embedded values */
  const Record_Of_Type* embval_array;
  /** Stores the erroneous descriptor of the embedded values field (for negative tests) */
  const Erroneous_descriptor_t* embval_err;
  /** Error value index for the embedded values (for negative tests) */
  int embval_err_val_idx;
  /** Erroneous descriptor index for the embedded values (for negative tests) */
  int embval_err_descr_idx;
#else
  /** Stores the array of embedded values (regular record-of) */
  const PreGenRecordOf::PREGEN__RECORD__OF__UNIVERSAL__CHARSTRING* embval_array_reg;
  /** Stores the array of embedded values (optimized record-of) */
  const PreGenRecordOf::PREGEN__RECORD__OF__UNIVERSAL__CHARSTRING__OPTIMIZED* embval_array_opt;
#endif
  /** Stores the index of the next embedded value to be read */
  int embval_index;
};

/** Information related to the embedded values in XML decoding
  * 
  * Used when a record/set with the EMBED-VALUES coding instruction contains
  * one or more record of/set of fields. */
struct embed_values_dec_struct_t
{
#ifdef TITAN_RUNTIME_2
  /** Stores the array of embedded values */
  Record_Of_Type* embval_array;
#else
  /** Stores the array of embedded values (regular record-of) */
  PreGenRecordOf::PREGEN__RECORD__OF__UNIVERSAL__CHARSTRING* embval_array_reg;
  /** Stores the array of embedded values (optimized record-of) */
  PreGenRecordOf::PREGEN__RECORD__OF__UNIVERSAL__CHARSTRING__OPTIMIZED* embval_array_opt;
#endif
  /** Stores the number of embedded values that are currently in the array,
    * and the index where the next one should be inserted */
  int embval_index;
};

/** Check the name of an XML node against a XER type descriptor.
 *
 * @param name the (local, unqualified) name of the XML element
 * @param p_td the type descriptor
 * @param exer \c true if Extended XER decoding, \c false for Basic and Canonical XER
 * @return \c true if \p name corresponds to the type descriptor, \c false otherwise.
 */
inline boolean check_name(const char *name, const XERdescriptor_t& p_td, boolean exer)
{
  return strncmp(name, p_td.names[exer], p_td.namelens[exer]-2) == 0
    && name[p_td.namelens[exer]-2] == '\0';
}

/** Verify the namespace of an XML node against a XER type descriptor.
 *
 * @pre EXER decoding is in progress
 *
 * @param ns_uri the URI of the current node
 * @param p_td the type descriptor
 * @return \c true if \p ns_uri is NULL and the type has no namespace
 * or it's the default namespace.
 * @return \c true if \p ns_uri is not NULL and it matches the one referenced
 * by \p p_td.
 * @return \c false otherwise.
 */
boolean check_namespace(const char *ns_uri, const XERdescriptor_t& p_td);

/** Check that the current element matches the XER descriptor
 *
 * Calls TTCN_EncDec_ErrorContext::error() if it doesn't.
 *
 * @param reader XML reader
 * @param p_td XER descriptor
 * @param exer 0 for Basic/Canonical XER, 1 for EXER
 * @return the name of the current element
 */
const char* verify_name(XmlReaderWrap& reader, const XERdescriptor_t& p_td, boolean exer);

/** Check the end tag
 *
 * Calls verify_name(), then compares \a depth with the current XML depth
 * and calls TTCN_EncDec_ErrorContext::error() if they don't match.
 *
 * @param reader XML reader
 * @param p_td XER descriptor
 * @param depth XML tag depth (0 for top-level element)
 * @param exer 0 for Basic/Canonical XER, 1 for EXER
 */
void verify_end(XmlReaderWrap& reader, const XERdescriptor_t& p_td, const int depth, boolean exer);

class TTCN_Buffer;

/** Output the namespace prefix
 *
 * The namespace prefix is determined by the XER descriptor (@a my_module
 * and @a ns_index fields). It is not written if p_td.xer_bits has
 * FORM_UNQUALIFIED.
 *
 * @param p_td  XER descriptor
 * @param p_buf buffer to write into
 *
 * @pre the caller should check that E-XER encoding is in effect.
 */
void write_ns_prefix(const XERdescriptor_t& p_td, TTCN_Buffer& p_buf);

/** Return the namespace referred to by a prefix
 *
 * Finds the namespace specified by \a prefix in the module's namespace table
 * and returns its URI. Returns NULL if the namespace is not found.
 *
 * @param prefix namespace prefix to be found
 * @param p_td XER descriptor (contains the module to search in)
 */
const char* get_ns_uri_from_prefix(const char *prefix, const XERdescriptor_t& p_td);

/** Output the beginning of an XML attribute.
 *
 * Writes a space, the attribute name (from \p p_td), and the string "='".
 * @post the buffer is ready to receive the actual value
 *
 * @param p_td  XER descriptor (contains the attribute name)
 * @param p_buf buffer to write into
 */
inline void begin_attribute(const XERdescriptor_t& p_td, TTCN_Buffer& p_buf)
{
  p_buf.put_c(' ');
  write_ns_prefix(p_td, p_buf);
  p_buf.put_s((size_t)p_td.namelens[1]-2, (const unsigned char*)p_td.names[1]);
  p_buf.put_s((size_t)2, (const unsigned char*)"='");
}

/** Indent.
 *
 * @param buf buffer to write into.
 * @param level indent level
 *
 * Writes the appropriate amount of indentation into \p buf.
 *
 * Indentation is currently done with with tabs.
 * */
int do_indent(TTCN_Buffer& buf, int level);

/** Ensures that the anyElement or anyAttribute field respects its namespace
  * restrictions.
  * In case of "anyElement from ..." or "anyAttributes from ..." the namespace
  * needs to be in the specified list.
  * In case of "anyElement except ..." or "anyAttributes except ..." it cannot
  * match any of the namespaces from the list.
  * An invalid namespace causes a dynamic test case error.
  *
  * @param p_td type descriptor of the field in question, contains the list of
  * valid or invalid namespaces
  * @param p_xmlns constains the namespace in question
  */
void check_namespace_restrictions(const XERdescriptor_t& p_td, const char* p_xmlns);


#ifdef DEFINE_XER_STRUCT
# define XER_STRUCT2(type_name,xmlname) \
  extern const XERdescriptor_t type_name##_xer_ = { \
    { xmlname ">\n", xmlname ">\n" }, \
    { 2+sizeof(xmlname)-1, 2+sizeof(xmlname)-1 }, \
    0U, WHITESPACE_PRESERVE, NULL, NULL, 0, 0, NULL, NULL, -1, XSD_NONE }
// The compiler should fold the two identical strings into one

# define XER_STRUCT_COPY(cpy,original) \
  const XERdescriptor_t& cpy##_xer_ = original##_xer_
#else
/** Declare a XER structure.
 *  @param type_name the name of a Titan runtime class
 *  @param xmlname the XML tag name
 */
# define XER_STRUCT2(type_name,xmlname) extern const XERdescriptor_t type_name##_xer_
# define XER_STRUCT_COPY(cpy,original)  extern const XERdescriptor_t& cpy##_xer_
#endif

/** Declare a XER structure where the name of the type matches the tag */
#       define XER_STRUCT(name) XER_STRUCT2(name, #name)

/* XER descriptors for built-in types.
 * The XML tag names are defined in Table 4, referenced by clause
 * 11.25.2 (X.680/2002) or 12.36.2 (X.680/2008) */

// Types shared between ASN.1 and TTCN-3
XER_STRUCT2(BITSTRING, "BIT_STRING");
XER_STRUCT (BOOLEAN);
XER_STRUCT (CHARSTRING);
XER_STRUCT2(FLOAT, "REAL");
XER_STRUCT (INTEGER);
XER_STRUCT2(OBJID, "OBJECT_IDENTIFIER");
XER_STRUCT2(OCTETSTRING, "OCTET_STRING");
XER_STRUCT (UNIVERSAL_CHARSTRING);

XER_STRUCT(RELATIVE_OID);

// ASN.1 types

XER_STRUCT2(EMBEDDED_PDV, "SEQUENCE");
XER_STRUCT2(EMBEDDED_PDV_identification, "identification");
XER_STRUCT2(EMBEDDED_PDV_identification_sxs, "syntaxes");
XER_STRUCT2(EMBEDDED_PDV_identification_sxs_abs, "abstract");
XER_STRUCT2(EMBEDDED_PDV_identification_sxs_xfr, "transfer");
XER_STRUCT2(EMBEDDED_PDV_identification_sx , "syntax");
XER_STRUCT2(EMBEDDED_PDV_identification_pci, "presentation-context-id");
XER_STRUCT2(EMBEDDED_PDV_identification_cn , "context-negotiation");
XER_STRUCT2(EMBEDDED_PDV_identification_cn_pci , "presentation-context-id");
XER_STRUCT2(EMBEDDED_PDV_identification_cn_tsx , "transfer-syntax");
XER_STRUCT2(EMBEDDED_PDV_identification_ts , "transfer-syntax");
XER_STRUCT2(EMBEDDED_PDV_identification_fix, "fixed");
XER_STRUCT2(EMBEDDED_PDV_data_value_descriptor, "data-value-descriptor");
XER_STRUCT2(EMBEDDED_PDV_data_value, "data-value");


XER_STRUCT2(EXTERNAL, "SEQUENCE");
XER_STRUCT2(EXTERNAL_direct_reference  , "direct-reference");
XER_STRUCT2(EXTERNAL_indirect_reference, "indirect-reference");
XER_STRUCT2(EXTERNAL_data_value_descriptor, "data-value-descriptor");
XER_STRUCT2(EXTERNAL_encoding, "encoding");
XER_STRUCT2(EXTERNAL_encoding_singleASN    , "single-ASN1-type");
XER_STRUCT2(EXTERNAL_encoding_octet_aligned, "octet-aligned");
XER_STRUCT2(EXTERNAL_encoding_arbitrary    , "arbitrary");

// The big, scary ASN.1 unrestricted character string
XER_STRUCT2(CHARACTER_STRING, "SEQUENCE");
XER_STRUCT_COPY(CHARACTER_STRING_identification,         EMBEDDED_PDV_identification);
XER_STRUCT_COPY(CHARACTER_STRING_identification_sxs,     EMBEDDED_PDV_identification_sxs);
XER_STRUCT_COPY(CHARACTER_STRING_identification_sxs_abs, EMBEDDED_PDV_identification_sxs_abs);
XER_STRUCT_COPY(CHARACTER_STRING_identification_sxs_xfr, EMBEDDED_PDV_identification_sxs_xfr);
XER_STRUCT_COPY(CHARACTER_STRING_identification_sx ,     EMBEDDED_PDV_identification_sx);
XER_STRUCT_COPY(CHARACTER_STRING_identification_pci,     EMBEDDED_PDV_identification_pci);
XER_STRUCT_COPY(CHARACTER_STRING_identification_cn ,     EMBEDDED_PDV_identification_cn);
XER_STRUCT_COPY(CHARACTER_STRING_identification_cn_pci , EMBEDDED_PDV_identification_cn_pci);
XER_STRUCT_COPY(CHARACTER_STRING_identification_cn_tsx , EMBEDDED_PDV_identification_cn_tsx);
XER_STRUCT_COPY(CHARACTER_STRING_identification_ts ,     EMBEDDED_PDV_identification_ts);
XER_STRUCT_COPY(CHARACTER_STRING_identification_fix,     EMBEDDED_PDV_identification_fix);
// this one is used for decoding only (only to check that it's absent)
XER_STRUCT2(CHARACTER_STRING_data_value_descriptor, "data-value-descriptor");
// this can not be folded with EMBEDDED-PDV
XER_STRUCT2(CHARACTER_STRING_data_value, "string-value");

// ASN.1 restricted character strings
XER_STRUCT(GeneralString);
XER_STRUCT(NumericString);
XER_STRUCT(UTF8String);
XER_STRUCT(PrintableString);
XER_STRUCT(UniversalString);

XER_STRUCT(BMPString);
XER_STRUCT(GraphicString);
XER_STRUCT(IA5String);
XER_STRUCT(TeletexString);
XER_STRUCT(VideotexString);
XER_STRUCT(VisibleString);

XER_STRUCT2(ASN_NULL, "NULL");
XER_STRUCT2(ASN_ROID, "RELATIVE_OID");
XER_STRUCT (ASN_ANY); // obsoleted by 2002

// TTCN-3 types
XER_STRUCT2(HEXSTRING, "hexstring");
XER_STRUCT2(VERDICTTYPE, "verdicttype");

/** @} */

#endif /*XER_HH_*/
