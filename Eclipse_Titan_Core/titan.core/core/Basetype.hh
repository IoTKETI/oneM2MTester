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
 *   Beres, Szabolcs
 *   Delic, Adam
 *   Forstner, Matyas
 *   Horvath, Gabriella
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#ifndef BASETYPE_HH
#define BASETYPE_HH

#include "Types.h"
#include "Encdec.hh"
#include "RInt.hh"
#include "JSON_Tokenizer.hh"
#include "Logger.hh"
#include "Error.hh"
#ifdef TITAN_RUNTIME_2
#include "Struct_of.hh"
#include "XER.hh"
#include "Vector.hh"
#include "RefdIndex.hh"
#endif

struct ASN_BERdescriptor_t;
struct ASN_BER_TLV_t;
struct TTCN_RAWdescriptor_t;
struct TTCN_TEXTdescriptor_t;
struct XERdescriptor_t;
struct TTCN_JSONdescriptor_t;
class  XmlReaderWrap;
class Module_Param;
class Module_Param_Name;
struct embed_values_enc_struct_t;
struct embed_values_dec_struct_t;

/** @brief Type descriptor
 *
 * There is one type descriptor object for each type.
 * Descriptors for built-in types are supplied by the runtime.
 * Descriptors for user-defined types are written by the compiler
 * in the generated code.
 */
struct TTCN_Typedescriptor_t {
  const char * const name; /**< Name of the type, e.g INTEGER, REAL, verdicttype, etc */
  const ASN_BERdescriptor_t * const ber; /**< Information for BER coding */
  const TTCN_RAWdescriptor_t * const raw; /**< Information for RAW coding */
  const TTCN_TEXTdescriptor_t * const text; /**< Information for TEXT coding */
  const XERdescriptor_t * const xer; /**< Information for XER */
  const TTCN_JSONdescriptor_t * const json; /**< Information for JSON coding */
  const TTCN_Typedescriptor_t * const oftype_descr; /**< Record-of element's type descriptor */
  /** ASN subtype
   *
   *  Used by classes implementing more than one ASN.1 type
   *  (OBJID and UNIVERSAL_CHARSTRING).
   */
  enum {
    DONTCARE,
    UNIVERSALSTRING, BMPSTRING, UTF8STRING,
    TELETEXSTRING, VIDEOTEXSTRING, GRAPHICSTRING, GENERALSTRING,
    OBJID, ROID
  } const asnbasetype;
};

#ifdef TITAN_RUNTIME_2

struct Erroneous_value_t {
  const boolean raw;
  const Base_Type * const errval; // NULL if `omit'
  const TTCN_Typedescriptor_t* type_descr; // NULL if `omit' or raw
};

struct Erroneous_values_t {
  const int field_index;
  const char* field_qualifier;
  const Erroneous_value_t * const before;
  const Erroneous_value_t * const value;
  const Erroneous_value_t * const after;
};

struct Erroneous_descriptor_t {
  const int field_index;
  const int omit_before; // -1 if none
  const char* omit_before_qualifier; // NULL if none
  const int omit_after;  // -1 if none
  const char* omit_after_qualifier; // NULL if none
  const int values_size;
  const Erroneous_values_t * const values_vec;
  const int embedded_size;
  const Erroneous_descriptor_t * const embedded_vec;
  /** search in values_vec for the field with index field_idx */
  const Erroneous_values_t* get_field_err_values(int field_idx) const;
  /** search in embedded_vec for the field with index field_idx */
  const Erroneous_descriptor_t* get_field_emb_descr(int field_idx) const;
  /** if the current element of values_vec has index field_idx then returns it and increments values_idx */
  const Erroneous_values_t* next_field_err_values(const int field_idx, int& values_idx) const;
  /** if the current element of embedded_vec has index field_idx then returns it and increments edescr_idx */
  const Erroneous_descriptor_t* next_field_emb_descr(const int field_idx, int& edescr_idx) const;
  void log() const;
  void log_() const;
};

#endif

class TTCN_Type_list;
class RAW_enc_tree;
class Limit_Token_List;
#ifdef TITAN_RUNTIME_2
class Text_Buf;
#endif

#ifdef TITAN_RUNTIME_2
#define VIRTUAL_IF_RUNTIME_2 virtual
#else
#define VIRTUAL_IF_RUNTIME_2
#endif

/**
 * The base class for all types in TTCN-3 runtime environment.
 *
 * Uses the compiler-generated default constructor, copy constructor
 * and assignment operator (they do nothing because the class is empty).
 */
class Base_Type {
public:

  /** Whether the value is present.
   * Note: this is not the TTCN-3 ispresent(), kept for backward compatibility!
   * causes DTE, must be used only if the field is OPTIONAL<>
   */
  VIRTUAL_IF_RUNTIME_2 boolean ispresent() const;

  VIRTUAL_IF_RUNTIME_2 void log() const;

  /** Check whether the XML encoding of the type can begin with an XML element
   * having the specified name and namespace.
   *
   * This function checks for the "starter tag" of the type. This usually
   * means the tag should match the name in the XER descriptor,
   * with a few notable exceptions:
   *
   * - If the type is untagged, then
   *   - if it's a record, all fields up to the the first non-optional field
   *     are checked;
   *   - if it's a record-of, the element is checked.
   * - A \c universal \c charstring with ANY-ELEMENT can begin with any tag.
   *
   * @param name the XML element name (local name)
   * @param uri  the namespace URI of the element
   * @param xd XER descriptor
   * @param flavor one or more bits from XER_flavor. Only XER_EXTENDED counts
   * @return TRUE if the XML element can begin the XER encoding of the type,
   *         FALSE otherwise.
   */
  static boolean can_start(const char *name, const char *uri,
    XERdescriptor_t const& xd, unsigned int flavor, unsigned int flavor2);

#ifdef TITAN_RUNTIME_2
  /** Initialize this object (or one of its fields/elements) with a 
    * module parameter value. The module parameter may contain references to
    * other module parameters or module parameter expressions, which are processed
    * by this method to calculated the final result.
    * @param param module parameter value (its ID specifies which object is to be set) */
  virtual void set_param(Module_Param& param) = 0;
  /** Create a module parameter value equivalent to this object (or one of its
    * fields/elements)
    * @param param_name module parameter ID, specifies which object to convert */
  virtual Module_Param* get_param(Module_Param_Name& param_name) const = 0;
  /** Whether the type is a sequence-of.
   * @return \c FALSE */
  virtual boolean is_seof() const { return FALSE; }

  virtual void encode_text(Text_Buf& text_buf) const = 0;
  virtual void decode_text(Text_Buf& text_buf) = 0;

  virtual boolean is_bound() const = 0;
  virtual boolean is_value() const { return is_bound(); }
  virtual void clean_up() = 0;

  /** returns if the value of this is equal to the value of the parameter. */
  virtual boolean is_equal(const Base_Type* other_value) const = 0;

  /** Performs: *this = *other_value.
   * Does not take ownership of \p other_value. */
  virtual void set_value(const Base_Type* other_value) = 0;

  /** creates a copy of this object, returns a pointer to it. */
  virtual Base_Type* clone() const = 0;

  /** return reference to the type descriptor object for this type. */
  virtual const TTCN_Typedescriptor_t* get_descriptor() const = 0;

  /** @name override in class OPTIONAL<T>
   * @{ */
  virtual boolean is_optional() const { return FALSE; }

  /** Is the optional value present or omit.
   *  @return \c true if present, \c false otherwise
   *  used for TTCN-3 ispresent()
   **/
  virtual boolean is_present() const;

  /** Access the embedded type of the optional.
   *
   * The implementation in Base_Type always causes a dynamic testcase error.
   */
  virtual Base_Type* get_opt_value();

  /** Access the embedded type of the optional.
   *
   * The implementation in Base_Type always causes a dynamic testcase error.
   */
  virtual const Base_Type* get_opt_value() const;

  /** Set the optional field to \c omit.
   * The implementation in Base_Type always causes a dynamic testcase error.
   */
  virtual void set_to_omit();

  /** Set the optional field to present.
   * The implementation in Base_Type always causes a dynamic testcase error.
   */
  virtual void set_to_present();
  /** @} */
  
  virtual ~Base_Type() { }

#endif

  /** Do nothing for non-structured types. */
  VIRTUAL_IF_RUNTIME_2 void set_implicit_omit() { }

  /** @brief Encode an instance of this object

  Description:

  @param p_td type descriptor
  @param p_buf buffer
  @param p_coding coding type to select BER, RAW or TEXT coding

  */
  VIRTUAL_IF_RUNTIME_2 void encode(const TTCN_Typedescriptor_t& p_td,
    TTCN_Buffer& p_buf, TTCN_EncDec::coding_t p_coding, ...) const;

  /** @brief Decode an instance of this object

  Description:

  @param p_td type descriptor
  @param p_buf buffer
  @param p_coding coding type to select BER, RAW or TEXT coding

  */
  VIRTUAL_IF_RUNTIME_2 void decode(const TTCN_Typedescriptor_t& p_td,
    TTCN_Buffer& p_buf, TTCN_EncDec::coding_t p_coding, ...);

protected:
  /** Check type descriptor for BER encoding
   *
   * @param p_td a TTCN type descriptor
   *
   * Calls TTCN_EncDec_ErrorContext::error_internal if
   * p_td has no BER descriptor.
   */
  static void BER_chk_descr(const TTCN_Typedescriptor_t& p_td);

  /** Check the BER coding
   *
   * If \p p_coding is either BER_ENCODE_CER or BER_ENCODE_DER,
   * this function does nothing. Otherwise, it issues a warning
   * and sets \p p_coding to BER_ENCODE_DER.
   *
   * @param[in, out] p_coding BER coding
   * @post p_coding is either BER_ENCODE_CER or BER_ENCODE_DER
   */
  static void BER_encode_chk_coding(unsigned& p_coding);

  /** Check type descriptor for XER encoding and the XER coding variant
   *
   * @param[in, out] p_coding XER coding
   * @param[in] p_td TTCN type descriptor
   *
   * If \p p_td has no XER descriptor, calls
   * TTCN_EncDec_ErrorContext::error_internal()
   *
   * If \p p_coding is one of XER_BASIC, XER_CANONICAL or XER_EXTENDED,
   * this function does nothing. Otherwise, it issues a warning
   * and sets \p p_coding to XER_BASIC.
   *
   * @post p_coding is one of XER_BASIC, XER_CANONICAL or XER_EXTENDED
   */
  static void XER_encode_chk_coding(unsigned& p_coding,
    const TTCN_Typedescriptor_t& p_td);

  /** Check bound and create a 0-length TLV in case unbound is ignored.
   *
   *  @param[in] p_isbound
   *  @return a newly constructed and empty \c ASN_BER_TLV_t
   *          if p_isbound is false, or NULL if p_isbound is true
   *  @note if p_isbound is false, this function calls
   *  TTCN_EncDec_ErrorContext::error with TTCN_EncDec::ET_UNBOUND.
   *  If that error is not ignored, this function does not return at all.
   */
  static ASN_BER_TLV_t* BER_encode_chk_bound(boolean p_isbound);

private:
  static void BER_encode_putoctets_OCTETSTRING
  (unsigned char *target,
   unsigned int octetnum_start, unsigned int octet_count,
   int p_nof_octets, const unsigned char *p_octets_ptr);

protected:
  static ASN_BER_TLV_t* BER_encode_TLV_OCTETSTRING
  (unsigned p_coding,
   int p_nof_octets, const unsigned char *p_octets_ptr);

  static ASN_BER_TLV_t *BER_encode_TLV_INTEGER(unsigned p_coding,
    const int_val_t& p_int_val);
  static ASN_BER_TLV_t *BER_encode_TLV_INTEGER(unsigned p_coding,
    const int& p_int_val);

  static void BER_encode_chk_enum_valid(const TTCN_Typedescriptor_t& p_td,
                                        boolean p_isvalid, int p_value);

  static void BER_decode_str2TLV(TTCN_Buffer& p_buf, ASN_BER_TLV_t& p_tlv,
                                 unsigned L_form);

  static boolean BER_decode_constdTLV_next(const ASN_BER_TLV_t& p_tlv,
                                           size_t& V_pos,
                                           unsigned L_form,
                                           ASN_BER_TLV_t& p_target_tlv);

  static void BER_decode_constdTLV_end(const ASN_BER_TLV_t& p_tlv,
                                       size_t& V_pos,
                                       unsigned L_form,
                                       ASN_BER_TLV_t& p_target_tlv,
                                       boolean tlv_present);

  static void BER_decode_strip_tags(const ASN_BERdescriptor_t& p_ber,
                                    const ASN_BER_TLV_t& p_tlv,
                                    unsigned L_form,
                                    ASN_BER_TLV_t& stripped_tlv);

private:
  static void BER_decode_getoctets_OCTETSTRING
  (const unsigned char *source, size_t s_len,
   unsigned int& octetnum_start,
   int& p_nof_octets, unsigned char *p_octets_ptr);

protected:
  static void BER_decode_TLV_OCTETSTRING
  (const ASN_BER_TLV_t& p_tlv, unsigned L_form,
   unsigned int& octetnum_start,
   int& p_nof_octets, unsigned char *p_octets_ptr);

  static boolean BER_decode_TLV_INTEGER(const ASN_BER_TLV_t& p_tlv,
    unsigned L_form, int_val_t& p_int_val);
  static boolean BER_decode_TLV_INTEGER(const ASN_BER_TLV_t& p_tlv,
    unsigned L_form, int& p_int_val);

  static boolean BER_decode_TLV_CHOICE(const ASN_BERdescriptor_t& p_ber,
                                       const ASN_BER_TLV_t& p_tlv,
                                       unsigned L_form,
                                       ASN_BER_TLV_t& p_target_tlv);

  static boolean BER_decode_CHOICE_selection(boolean select_result,
                                             const ASN_BER_TLV_t& p_tlv);

  static void BER_decode_chk_enum_valid(const TTCN_Typedescriptor_t& p_td,
                                        boolean p_isvalid, int p_value);

public:
  VIRTUAL_IF_RUNTIME_2 ASN_BER_TLV_t* BER_encode_TLV(
    const TTCN_Typedescriptor_t& p_td, unsigned p_coding) const;
#ifdef TITAN_RUNTIME_2
  virtual ASN_BER_TLV_t* BER_encode_TLV_negtest(const Erroneous_descriptor_t* /*p_err_descr*/,
    const TTCN_Typedescriptor_t& /*p_td*/, unsigned /*p_coding*/) const;
  virtual ASN_BER_TLV_t* BER_encode_negtest_raw() const;
  virtual int encode_raw(TTCN_Buffer& p_buf) const;
  virtual int RAW_encode_negtest_raw(RAW_enc_tree& p_myleaf) const;
  virtual int JSON_encode_negtest_raw(JSON_Tokenizer&) const;
#endif

  /** Examines whether this message corresponds the tags in the
   * descriptor. If the BER descriptor contains no tags, then returns
   * TRUE. Otherwise, examines the first (outermost) tag only. */
#ifdef TITAN_RUNTIME_2
  virtual
#else
  static
#endif
  boolean BER_decode_isMyMsg(const TTCN_Typedescriptor_t& p_td,
                                    const ASN_BER_TLV_t& p_tlv);

  /** Returns TRUE on success, FALSE otherwise. */
  VIRTUAL_IF_RUNTIME_2 boolean BER_decode_TLV(const TTCN_Typedescriptor_t& p_td,
    const ASN_BER_TLV_t& p_tlv, unsigned L_form);

  VIRTUAL_IF_RUNTIME_2 void BER_decode_opentypes(
    TTCN_Type_list& /*p_typelist*/, unsigned /*L_form*/) {}

  /** Encode with RAW coding.
   * @return the length of the encoding
   * @param p_td type descriptor
   * @param myleaf filled with RAW encoding data
   * @note Basetype::RAW_encode throws an error. */
  VIRTUAL_IF_RUNTIME_2 int RAW_encode(const TTCN_Typedescriptor_t& p_td,
                                      RAW_enc_tree& myleaf) const;
#ifdef TITAN_RUNTIME_2
  virtual int RAW_encode_negtest(const Erroneous_descriptor_t *p_err_descr,
    const TTCN_Typedescriptor_t& p_td, RAW_enc_tree& myleaf) const;
#endif

  /** Decode with RAW coding
   *
   * @param p_td type descriptor
   * @param p_buf buffer with data to be decoded
   * @param limit number of bits the decoder is allowed to use. At the top level
   *        this is 8x the number of bytes in the buffer.
   * @param top_bit_ord (LSB/MSB) from TTCN_RAWdescriptor_t::top_bit_order
   * @param no_err set to TRUE if the decoder is to return errors silently,
   *        without calling TTCN_EncDec_ErrorContext::error
   * @param sel_field selected field indicator for CROSSTAG, or -1
   * @param first_call default TRUE. May be FALSE for a REPEATABLE record-of
   *        inside a set, if an element has been successfully decoded.
   * @return length of decoded field, or a negative number for error
   * @note Basetype::RAW_decode throws an error. */
  VIRTUAL_IF_RUNTIME_2 int RAW_decode(
    const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf, int limit,
    raw_order_t top_bit_ord, boolean no_err=FALSE, int sel_field=-1,
    boolean first_call=TRUE);


  /** Encode with TEXT encoding.
   * @return the length of the encoding
   * @param p_td type descriptor
   * @param p_buf buffer for the encoded data
   * @note Basetype::TEXT_encode throws an error. */
  VIRTUAL_IF_RUNTIME_2 int TEXT_encode(const TTCN_Typedescriptor_t& p_td,
                                       TTCN_Buffer& p_buf) const;

#ifdef TITAN_RUNTIME_2
  /** Encode with TEXT encoding negative test.
     * @return the length of the encoding
     * @param p_err_descr type descriptor
     * @param p_td type descriptor
     * @param p_buf buffer for the encoded data
     * @note Basetype::TEXT_encode_negtest throws an error. */
  virtual int TEXT_encode_negtest(const Erroneous_descriptor_t* p_err_descr, const TTCN_Typedescriptor_t& p_td,
          TTCN_Buffer& p_buf) const;
#endif

  /** Decode TEXT.
   * @return decoded length
   * @note Basetype::TEXT_decode throws an error. */
  VIRTUAL_IF_RUNTIME_2 int TEXT_decode(
    const TTCN_Typedescriptor_t&, TTCN_Buffer&, Limit_Token_List&,
    boolean no_err=FALSE, boolean first_call=TRUE);

  /** Write the XER encoding of the current object into the buffer.
   *
   * @param p_td type descriptor
   * @param p_buf buffer
   * @param flavor one of XER_flavor values
   * @param indent indentation level
   * @param emb_val embed values data (only relevant for record of types)
   * @return number of bytes written into the buffer
   */
  VIRTUAL_IF_RUNTIME_2 int XER_encode(const XERdescriptor_t& p_td,
    TTCN_Buffer& p_buf, unsigned int flavor, unsigned int flavor2, int indent, embed_values_enc_struct_t* emb_val) const;

#ifdef TITAN_RUNTIME_2
  virtual int XER_encode_negtest(const Erroneous_descriptor_t* /*p_err_descr*/,
    const XERdescriptor_t& /*p_td*/, TTCN_Buffer& /*p_buf*/,
    unsigned int /*flavor*/, unsigned int /*flavor2*/, int /*indent*/, embed_values_enc_struct_t* /*emb_val*/) const;
#endif

  /** Decode the current object from the supplied buffer.
   *
   * The XML pull parser presents a forward iterator over the XML elements.
   * @pre Upon entering this function, the current node should be the one corresponding
   * to this object, or possibly a text or whitespace node before it.
   * Code should begin like this:
   * @code
   * int Foo::XER_decode() {
   *   int success=1, type;
   *   for ( ; success==1; success=reader.Read() ) {
   *     int type=reader.NodeType();
   *     if (XML_READER_TYPE_ELEMENT==type) {
   *       verify_name(
   *     }
   *   }
   * }
   * @endcode
   *
   * @post Upon leaving the function, the current node should be \b past
   * the end tag, i.e. the next start tag (or any whitespace before it).
   *
   * It is important not to advance the parser too far, to avoid "stealing"
   * the content of other fields (the parser can only go forward).
   *
   * @param p_td type descriptor
   * @param reader Wrapper around the XML processor
   * @param flavor one of XER_flavor values
   * @param flavor2 one of XER_flavor2 values
   * @param emb_val embed values data (only relevant for record of types)
   * @return number of bytes "consumed"
   */
  VIRTUAL_IF_RUNTIME_2 int XER_decode(const XERdescriptor_t& p_td,
    XmlReaderWrap& reader, unsigned int flavor, unsigned int flavor2, embed_values_dec_struct_t* emb_val);

  /** Return an array of namespace declarations.
   *
   * @param[in] p_td XER descriptor of the type.
   * @param[out] num set to the number of strings returned.
   * @param[out] def_ns set to @p true if a default namespace was encountered
   * @return an array of strings or NULL. All the strings in the array
   * and the array itself must be deallocated by the caller (if num>0).
   *
   * The strings are in the following format:
   * @code " xmlns:prefix='uri'" @endcode
   * (the space at start allows direct concatenation of the strings)
   */
  VIRTUAL_IF_RUNTIME_2 char ** collect_ns(const XERdescriptor_t& p_td,
    size_t& num, bool& def_ns, unsigned int flavor = 0) const;

  /** Copy strings from @p new_namespaces to @p collected_ns,
   *  discarding duplicates.
   *
   * May reallocate @p collected_ns, in which case @p num_collected
   * is adjusted accordingly.
   *
   * @param collected_ns target array
   * @param num_collected number of already collected namespaces
   * @param new_namespaces new array
   * @param num_new number of new namespaces
   *
   * @post new_namespaces has been deallocated and set to NULL.
   */
  static void merge_ns(char **&collected_ns, size_t& num_collected,
    char **new_namespaces, size_t num_new);

  typedef char** (Base_Type::*collector_fn)
    (const XERdescriptor_t& p_td, size_t& num, bool& def_ns, unsigned int flavor) const;

  /** Start writing the XML representation
   *
   * @param[in]  p_td XER descriptor
   * @param[out] p_buf buffer to write into
   * @param[in,out] flavor XER variant (basic, canonical or extended),
   *        also used to pass various flags between begin_xml and its caller
   * @param[in]  indent indentation level
   * @param[in]  empty true if an empty-element tag is needed
   * @param[in]  collector namespace collector function
   * @param[in]  type_atr type identification attribute for useNil/useUnion,
   *        in the following format: "xsi:type='foo'"
   * @return "omit_tag": zero if the type's own tag was written (not omitted
   *        e.g. due to untagged).
   *        If the XML tag was omitted, the return value is nonzero.
   *        The special value -1 is returned if the XML in the buffer
   *        was shortened by exactly one character.
   */
  VIRTUAL_IF_RUNTIME_2 int begin_xml(const XERdescriptor_t& p_td, TTCN_Buffer& p_buf,
    unsigned int& flavor, int indent, boolean empty,
    collector_fn collector = &Base_Type::collect_ns, const char *type_atr = NULL, unsigned int flavor2 = 0) const;
  /** Finish the XML representation.
   *
   * @param[in]  p_td XER descriptor
   * @param[out] p_buf buffer to write into
   * @param[in]  flavor XER variant (basic, canonical or extended),
   *        also used to pass various flags
   * @param[in]  indent indentation level
   * @param[in]  empty true if an empty-element tag is needed
   */
  VIRTUAL_IF_RUNTIME_2 void end_xml  (const XERdescriptor_t& p_td, TTCN_Buffer& p_buf,
    unsigned int flavor, int indent, boolean empty, unsigned int flavor2 = 0) const;
  
  /** Encode JSON.
   * @return encoded length
   * @note Basetype::JSON_encode throws an error. */
  VIRTUAL_IF_RUNTIME_2 int JSON_encode(const TTCN_Typedescriptor_t& p_td, JSON_Tokenizer&) const;
  
#ifdef TITAN_RUNTIME_2
  /** Encode with JSON encoding negative test.
    * @return the length of the encoding
    * @param p_err_descr erroneous type descriptor
    * @param p_td type descriptor
    * @param p_tok JSON tokenizer for the encoded data
    * @note Basetype::JSON_encode_negtest throws an error. */
  virtual int JSON_encode_negtest(const Erroneous_descriptor_t* p_err_descr, const TTCN_Typedescriptor_t& p_td,
          JSON_Tokenizer& p_tok) const;
#endif
  
  /** Decode JSON.
   * @return decoded length
   * @note Basetype::JSON_decode throws an error. */
  VIRTUAL_IF_RUNTIME_2 int JSON_decode(const TTCN_Typedescriptor_t& p_td, JSON_Tokenizer&, boolean);
  
  /** not a component by default (components will return true) */
  inline boolean is_component() { return FALSE; }
};

/**
 * This class is used by BER decoding to handle
 * ComponentRelationConstraint.
 */
class TTCN_Type_list {
private:
  size_t n_types;
  const Base_Type **types;
  /// Copy constructor disabled
  TTCN_Type_list(const TTCN_Type_list&);
  /// Assignment disabled
  TTCN_Type_list& operator=(const TTCN_Type_list&);
public:
  TTCN_Type_list() : n_types(0), types(NULL) {}
  ~TTCN_Type_list();
  void push(const Base_Type *p_type);
  const Base_Type* pop();
  /** If \a pos is 0 then returns the outermost, otherwise the "pos"th
   * inner parent. */
  const Base_Type* get_nth(size_t pos) const;
};

/** @name Type descriptor objects
@{
*/
extern const TTCN_Typedescriptor_t BOOLEAN_descr_;
extern const TTCN_Typedescriptor_t INTEGER_descr_;
extern const TTCN_Typedescriptor_t FLOAT_descr_;
extern const TTCN_Typedescriptor_t VERDICTTYPE_descr_;
extern const TTCN_Typedescriptor_t OBJID_descr_;
extern const TTCN_Typedescriptor_t BITSTRING_descr_;
extern const TTCN_Typedescriptor_t HEXSTRING_descr_;
extern const TTCN_Typedescriptor_t OCTETSTRING_descr_;
extern const TTCN_Typedescriptor_t CHARSTRING_descr_;
extern const TTCN_Typedescriptor_t UNIVERSAL_CHARSTRING_descr_;
extern const TTCN_Typedescriptor_t COMPONENT_descr_;
extern const TTCN_Typedescriptor_t DEFAULT_descr_;
extern const TTCN_Typedescriptor_t ASN_NULL_descr_;
extern const TTCN_Typedescriptor_t ASN_ANY_descr_;
extern const TTCN_Typedescriptor_t EXTERNAL_descr_;
extern const TTCN_Typedescriptor_t EMBEDDED_PDV_descr_;
extern const TTCN_Typedescriptor_t CHARACTER_STRING_descr_;
extern const TTCN_Typedescriptor_t ObjectDescriptor_descr_;
extern const TTCN_Typedescriptor_t UTF8String_descr_;
extern const TTCN_Typedescriptor_t ASN_ROID_descr_;
extern const TTCN_Typedescriptor_t NumericString_descr_;
extern const TTCN_Typedescriptor_t PrintableString_descr_;
extern const TTCN_Typedescriptor_t TeletexString_descr_;
extern const TTCN_Typedescriptor_t& T61String_descr_;
extern const TTCN_Typedescriptor_t VideotexString_descr_;
extern const TTCN_Typedescriptor_t IA5String_descr_;
extern const TTCN_Typedescriptor_t ASN_GeneralizedTime_descr_;
extern const TTCN_Typedescriptor_t ASN_UTCTime_descr_;
extern const TTCN_Typedescriptor_t GraphicString_descr_;
extern const TTCN_Typedescriptor_t VisibleString_descr_;
extern const TTCN_Typedescriptor_t& ISO646String_descr_;
extern const TTCN_Typedescriptor_t GeneralString_descr_;
extern const TTCN_Typedescriptor_t UniversalString_descr_;
extern const TTCN_Typedescriptor_t BMPString_descr_;
/** @} */

#ifdef TITAN_RUNTIME_2

class Text_Buf;
class INTEGER;

/** The base class of all record-of types when using RT2
 *
 */
// Record_Of_Template can be found in Template.hh
class Record_Of_Type : public Base_Type, public RefdIndexInterface
{
  friend class Set_Of_Template;
  friend class Record_Of_Template;
protected:
  friend boolean operator==(null_type null_value,
                            const Record_Of_Type& other_value);
  struct recordof_setof_struct {
    int ref_count;
    int n_elements;
    Base_Type **value_elements;
  } *val_ptr;
  Erroneous_descriptor_t* err_descr;
  
  struct refd_index_struct {
    /** Stores the indices of elements that are referenced by 'out' and 'inout' parameters.
      * These elements must not be deleted.*/
    Vector<int> refd_indices;

    /** Cached maximum value of \a refd_indices (default: -1).*/
    int max_refd_index;
  } *refd_ind_ptr;
  
  static boolean compare_function(const Record_Of_Type *left_ptr, int left_index, const Record_Of_Type *right_ptr, int right_index);
  Record_Of_Type() : val_ptr(NULL), err_descr(NULL), refd_ind_ptr(NULL) {}
  Record_Of_Type(null_type other_value);
  Record_Of_Type(const Record_Of_Type& other_value);
  /// Assignment disabled
  Record_Of_Type& operator=(const Record_Of_Type& other_value);
  
  /** Returns the number of actual elements in the record of (does not count the
    * unbound elements left at the end of the record of struct to make sure
    * referenced elements are not deleted). */
  int get_nof_elements() const;
  
  /** Returns true if the indexed element is bound */
  boolean is_elem_bound(int index) const;
  
  /** Returns the highest referenced index (uses \a max_refd_index as its cache)*/
  int get_max_refd_index();
  
  /** Returns true if the element at the given index is referenced by an 'out' or
    * 'inout' parameter. */
  boolean is_index_refd(int index);

public:
  void set_val(null_type other_value);

  boolean operator==(null_type other_value) const;
  boolean operator!=(null_type other_value) const;

  Base_Type* get_at(int index_value);
  Base_Type* get_at(const INTEGER& index_value);
  const Base_Type* get_at(int index_value) const;
  const Base_Type* get_at(const INTEGER& index_value) const;

  virtual boolean is_equal(const Base_Type* other_value) const;
  virtual void set_value(const Base_Type* other_value);

  /* following functions return rec_of or this or other_value */
  Record_Of_Type* rotl(const INTEGER& rotate_count, Record_Of_Type* rec_of) const;
  Record_Of_Type* rotr(int rotate_count, Record_Of_Type* rec_of) const;
  Record_Of_Type* rotr(const INTEGER& rotate_count, Record_Of_Type* rec_of) const;
  Record_Of_Type* concat(const Record_Of_Type* other_value, Record_Of_Type* rec_of) const;
  /* parameter rec_of must be a newly created descendant object, these helper functions fill it */
  void substr_(int index, int returncount, Record_Of_Type* rec_of) const;
  void replace_(int index, int len, const Record_Of_Type* repl, Record_Of_Type* rec_of) const;
  void replace_(int index, int len, const Record_Of_Template* repl, Record_Of_Type* rec_of) const;
  void replace_(int index, int len, const Set_Of_Template* repl, Record_Of_Type* rec_of) const;

  void set_size(int new_size);

  /** Implementation for isbound.
   * @return \c true if the Record_Of itself is bound, \c false otherwise.
   * Ignores elements. No need to be overridden. */
  virtual boolean is_bound() const;
  /** Implementation for isvalue.
   * @return \c true if all elements are bound, \c false otherwise */
  virtual boolean is_value() const;
  virtual void clean_up();

  virtual boolean is_seof() const { return TRUE; }

  /** Implementation for the \c sizeof operation
   * @return the number of elements */
  int size_of() const;
  int n_elem() const { return size_of(); }

  /** Implementation for the lengthof operation.
   * @return the index of the last bound element +1 */
  int lengthof() const;
  virtual void log() const;
  virtual void set_param(Module_Param& param);
  virtual Module_Param* get_param(Module_Param_Name& param_name) const;
  virtual void set_implicit_omit();
  virtual void encode_text(Text_Buf& text_buf) const;
  virtual void decode_text(Text_Buf& text_buf);

  virtual void BER_decode_opentypes(TTCN_Type_list& p_typelist, unsigned L_form);

  virtual void encode(const TTCN_Typedescriptor_t&, TTCN_Buffer&, TTCN_EncDec::coding_t, ...) const;
  virtual void decode(const TTCN_Typedescriptor_t&, TTCN_Buffer&, TTCN_EncDec::coding_t, ...);

  virtual ASN_BER_TLV_t* BER_encode_TLV(const TTCN_Typedescriptor_t& p_td, unsigned p_coding) const;
  virtual ASN_BER_TLV_t* BER_encode_TLV_negtest(const Erroneous_descriptor_t* p_err_descr, const TTCN_Typedescriptor_t& p_td, unsigned p_coding) const;
  virtual boolean BER_decode_TLV(const TTCN_Typedescriptor_t& p_td, const ASN_BER_TLV_t& p_tlv, unsigned L_form);

  virtual int RAW_encode_negtest(const Erroneous_descriptor_t *p_err_descr, const TTCN_Typedescriptor_t& p_td, RAW_enc_tree& myleaf) const;
  virtual int RAW_encode(const TTCN_Typedescriptor_t&, RAW_enc_tree&) const;
  /** Decode RAW
   *
   * @param td type descriptor
   * @param buf buffer
   * @param limit max number of bits that can be used
   * @param top_bit_ord
   * @param no_err (not used)
   * @param sel_field
   * @param first_call default true, false for second and subsequent calls
   *        of a REPEATABLE record-of. Equivalent to "not repeated"
   * @return number of bits used, or a negative number in case of error
   */
  virtual int RAW_decode(const TTCN_Typedescriptor_t& td, TTCN_Buffer& buf, int limit,
    raw_order_t top_bit_ord, boolean no_err=FALSE, int sel_field=-1, boolean first_call=TRUE);
  /// raw extension bit
  virtual int rawdec_ebv() const;

  virtual int TEXT_encode(const TTCN_Typedescriptor_t&, TTCN_Buffer&) const;
  virtual int TEXT_encode_negtest(const Erroneous_descriptor_t* p_err_descr, const TTCN_Typedescriptor_t&, TTCN_Buffer&) const;
  virtual int TEXT_decode(const TTCN_Typedescriptor_t&, TTCN_Buffer&, Limit_Token_List&, boolean no_err=FALSE, boolean first_call=TRUE);

  virtual int XER_encode(const XERdescriptor_t& p_td, TTCN_Buffer& p_buf,
    unsigned int flavor, unsigned int flavor2, int indent, embed_values_enc_struct_t*) const;
  virtual int XER_encode_negtest(const Erroneous_descriptor_t* p_err_descr,
    const XERdescriptor_t& p_td, TTCN_Buffer& p_buf, unsigned flavor, unsigned flavor2, int indent, embed_values_enc_struct_t*) const;
  /// Helper for XER_encode_negtest
  int encode_element(int i, const XERdescriptor_t& p_td, const Erroneous_values_t* err_vals,
    const Erroneous_descriptor_t* emb_descr,
    TTCN_Buffer& p_buf, unsigned int flavor, unsigned int flavor2, int indent, embed_values_enc_struct_t* emb_val) const;
  virtual int XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& reader, unsigned int, unsigned int, embed_values_dec_struct_t*);
  virtual boolean isXerAttribute() const;
  virtual boolean isXmlValueList() const;
  
  /** Encodes accordingly to the JSON encoding rules.
    * Returns the length of the encoded data. */
  int JSON_encode(const TTCN_Typedescriptor_t&, JSON_Tokenizer&) const;
  
  /** Negative testing for the JSON encoder
    * Encodes this value according to the JSON encoding rules, but with the
    * modifications (errors) specified in the erroneous descriptor parameter. */
  int JSON_encode_negtest(const Erroneous_descriptor_t*, const TTCN_Typedescriptor_t&, JSON_Tokenizer&) const;
  
  /** Decodes accordingly to the JSON encoding rules.
    * Returns the length of the decoded data. */
  int JSON_decode(const TTCN_Typedescriptor_t&, JSON_Tokenizer&, boolean);

  /** @returns \c true  if this is a set-of type,
   *           \c false if this is a record-of type */
  virtual boolean is_set() const = 0;
  /** creates an instance of the record's element class, using the default constructor */
  virtual Base_Type* create_elem() const = 0;

  /** Constant unbound element - return this instead of NULL in constant functions */
  virtual const Base_Type* get_unbound_elem() const = 0;

  virtual char ** collect_ns(const XERdescriptor_t& p_td, size_t& num, bool& def_ns, unsigned int flavor) const;
  virtual boolean can_start_v(const char *name, const char *prefix,
    XERdescriptor_t const& xd, unsigned int flavor, unsigned int flavor2) = 0;

  void set_err_descr(Erroneous_descriptor_t* p_err_descr) { err_descr=p_err_descr; }
  Erroneous_descriptor_t* get_err_descr() const { return err_descr; }
  
  /** Indicates that the element at the given index is referenced by an 'out' or
    * 'inout' parameter and must not be deleted.
    * Used just before the actual function call that references the element. */
  virtual void add_refd_index(int index);
  
  /** Indicates that the element at the given index is no longer referenced by
    * an 'out' or 'inout' parameter.
    * Used immediately after the actual function call that referenced the element. */
  virtual void remove_refd_index(int index);
};

extern boolean operator==(null_type null_value,
                          const Record_Of_Type& other_value);
extern boolean operator!=(null_type null_value,
                          const Record_Of_Type& other_value);

////////////////////////////////////////////////////////////////////////////////

/** The base class of all record types when using RT2  */
// Record_Template can be found in Template.hh
class Record_Type : public Base_Type {
protected:
  struct default_struct {
    int index;
    const Base_Type* value;
  };
  Erroneous_descriptor_t* err_descr;
public:
  Record_Type() : err_descr(NULL) {}

  /// @{
  /** get pointer to a field */
  virtual Base_Type* get_at(int index_value) = 0;
  virtual const Base_Type* get_at(int index_value) const = 0;
  /// @}

  /** get the index to a field based on its name and namespace URI, or -1 */
  int get_index_byname(const char *name, const char *uri) const;

  /** get number of fields */
  virtual int get_count() const = 0;
  /** number of optional fields */
  virtual int optional_count() const { return 0; }

  /** virtual functions inherited from Base_Type */
  boolean is_equal(const Base_Type* other_value) const;
  void set_value(const Base_Type* other_value);

  /** @returns \c true  if this is a set type,
   *           \c false if this is a record type */
  virtual boolean is_set() const = 0;

  /** return the name of a field */
  virtual const char* fld_name(int field_index) const = 0;

  /** return the descriptor of a field */
  virtual const TTCN_Typedescriptor_t* fld_descr(int field_index) const = 0;

  /** return int array which contains the indexes of optional fields
      and a -1 value appended to the end, or return NULL if no optional fields,
      generated classes override if there are optional fields */
  virtual const int* get_optional_indexes() const { return NULL; }
  /** default values and indexes, if no default fields then returns NULL,
      the last default_struct.index is -1 */
  virtual const default_struct* get_default_indexes() const { return NULL; }

  /** override if it's TRUE for the specific type (if sdef->opentype_outermost) */
  virtual boolean is_opentype_outermost() const { return FALSE; }

  virtual boolean default_as_optional() const { return FALSE; }

  virtual boolean is_bound() const;
  virtual boolean is_value() const;
  virtual void clean_up();
  virtual void log() const;
  virtual void set_param(Module_Param& param);
  virtual Module_Param* get_param(Module_Param_Name& param_name) const;
  virtual void set_implicit_omit();

  int size_of() const;
  virtual void encode_text(Text_Buf& text_buf) const;
  virtual void decode_text(Text_Buf& text_buf);

  virtual void encode(const TTCN_Typedescriptor_t&, TTCN_Buffer&, TTCN_EncDec::coding_t, ...) const;
  virtual void decode(const TTCN_Typedescriptor_t&, TTCN_Buffer&, TTCN_EncDec::coding_t, ...);

  virtual ASN_BER_TLV_t* BER_encode_TLV(const TTCN_Typedescriptor_t& p_td, unsigned p_coding) const;
  virtual ASN_BER_TLV_t* BER_encode_TLV_negtest(const Erroneous_descriptor_t* p_err_descr,
    const TTCN_Typedescriptor_t& p_td, unsigned p_coding) const;
  virtual boolean BER_decode_TLV(const TTCN_Typedescriptor_t& p_td, const ASN_BER_TLV_t& p_tlv, unsigned L_form);
  virtual void BER_decode_opentypes(TTCN_Type_list& p_typelist, unsigned L_form);

  virtual int RAW_encode(const TTCN_Typedescriptor_t&, RAW_enc_tree&) const;
  virtual int RAW_encode_negtest(const Erroneous_descriptor_t *, const TTCN_Typedescriptor_t&, RAW_enc_tree&) const;
  virtual int RAW_decode(const TTCN_Typedescriptor_t&, TTCN_Buffer&, int, raw_order_t, boolean no_err = FALSE, int sel_field = -1, boolean first_call = TRUE);
  // Helper functions for RAW enc/dec, shall be overridden in descendants if the default behavior is not enough.
  virtual boolean raw_has_ext_bit() const { return FALSE; }

  virtual int TEXT_encode(const TTCN_Typedescriptor_t&, TTCN_Buffer&) const;
  virtual int TEXT_encode_negtest(const Erroneous_descriptor_t* p_err_descr, const TTCN_Typedescriptor_t&, TTCN_Buffer&) const;
  virtual int TEXT_decode(const TTCN_Typedescriptor_t&, TTCN_Buffer&, Limit_Token_List&, boolean no_err=FALSE, boolean first_call=TRUE);

  virtual int XER_encode(const XERdescriptor_t& p_td, TTCN_Buffer& p_buf,
    unsigned int flavor, unsigned int flavor2, int indent, embed_values_enc_struct_t*) const;
  virtual int XER_encode_negtest(const Erroneous_descriptor_t* p_err_descr,
    const XERdescriptor_t& p_td, TTCN_Buffer& p_buf,
    unsigned int flavor, unsigned int flavor2, int indent, embed_values_enc_struct_t*) const;
  virtual int XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& reader,
    unsigned int, unsigned int, embed_values_dec_struct_t*);
  /// @{
  /// Methods overridden in the derived (generated) class
  virtual int get_xer_num_attr() const { return 0; /* default */ }
  virtual const XERdescriptor_t* xer_descr(int field_index) const;
  /// @}
  virtual char ** collect_ns(const XERdescriptor_t& p_td, size_t& num, bool& def_ns, unsigned int flavor = 0) const;
  virtual boolean can_start_v(const char *name, const char *prefix,
    XERdescriptor_t const& xd, unsigned int flavor, unsigned int flavor2) = 0;
  
  /** Encodes accordingly to the JSON encoding rules.
    * Returns the length of the encoded data. */
  int JSON_encode(const TTCN_Typedescriptor_t&, JSON_Tokenizer&) const;
  
  /** Negative testing for the JSON encoder
    * Encodes this value according to the JSON encoding rules, but with the
    * modifications (errors) specified in the erroneous descriptor parameter. */
  int JSON_encode_negtest(const Erroneous_descriptor_t*, const TTCN_Typedescriptor_t&, JSON_Tokenizer&) const;
  
  /** Decodes accordingly to the JSON encoding rules.
    * Returns the length of the decoded data. */
  int JSON_decode(const TTCN_Typedescriptor_t&, JSON_Tokenizer&, boolean);

  void set_err_descr(Erroneous_descriptor_t* p_err_descr) { err_descr=p_err_descr; }
  Erroneous_descriptor_t* get_err_descr() const { return err_descr; }
private:
  /// Helper for XER_encode_negtest
  int encode_field(int i, const Erroneous_values_t* err_vals, const Erroneous_descriptor_t* emb_descr,
  TTCN_Buffer& p_buf, unsigned int sub_flavor, unsigned int flavor2, int indent, embed_values_enc_struct_t* emb_val) const;
};

////////////////////////////////////////////////////////////////////////////////
class Empty_Record_Type : public Base_Type {
protected:
  boolean bound_flag;

  Empty_Record_Type();
  Empty_Record_Type(const Empty_Record_Type& other_value);

public:
  boolean operator==(null_type other_value) const;
  inline boolean operator!=(null_type other_value) const { return !(*this == other_value); }

  /** virtual functions inherited from Base_Type */
  virtual boolean is_equal(const Base_Type* other_value) const;
  virtual void set_value(const Base_Type* other_value);

  /** @returns \c true  if this is a set type,
   *           \c false if this is a record type */
  virtual boolean is_set() const = 0;

  inline void set_null() { bound_flag = TRUE; }
  virtual boolean is_bound() const { return bound_flag; }
  virtual void clean_up() { bound_flag = FALSE; }
  virtual void log() const;
  virtual void set_param(Module_Param& param);
  virtual Module_Param* get_param(Module_Param_Name& param_name) const;

  int size_of() const { return 0; }
  virtual void encode_text(Text_Buf& text_buf) const;
  virtual void decode_text(Text_Buf& text_buf);

  virtual void encode(const TTCN_Typedescriptor_t&, TTCN_Buffer&, TTCN_EncDec::coding_t, ...) const;
  virtual void decode(const TTCN_Typedescriptor_t&, TTCN_Buffer&, TTCN_EncDec::coding_t, ...);

  virtual ASN_BER_TLV_t* BER_encode_TLV(const TTCN_Typedescriptor_t& p_td, unsigned p_coding) const;
  virtual boolean BER_decode_TLV(const TTCN_Typedescriptor_t& p_td, const ASN_BER_TLV_t& p_tlv, unsigned L_form);

  virtual int RAW_encode(const TTCN_Typedescriptor_t&, RAW_enc_tree&) const;
  virtual int RAW_decode(const TTCN_Typedescriptor_t&, TTCN_Buffer&, int, raw_order_t, boolean no_err=FALSE, int sel_field=-1, boolean first_call=TRUE);

  virtual int TEXT_encode(const TTCN_Typedescriptor_t&, TTCN_Buffer&) const;
  virtual int TEXT_decode(const TTCN_Typedescriptor_t&, TTCN_Buffer&, Limit_Token_List&, boolean no_err=FALSE, boolean first_call=TRUE);

  virtual int XER_encode(const XERdescriptor_t& p_td, TTCN_Buffer& p_buf,
    unsigned int flavor, unsigned int flavor2, int indent, embed_values_enc_struct_t*) const;
  virtual int XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& reader,
    unsigned int, unsigned int, embed_values_dec_struct_t*);
  
  /** Encodes accordingly to the JSON encoding rules.
    * Returns the length of the encoded data. */
  virtual int JSON_encode(const TTCN_Typedescriptor_t&, JSON_Tokenizer&) const;
  
  /** Decodes accordingly to the JSON encoding rules.
    * Returns the length of the decoded data. */
  virtual int JSON_decode(const TTCN_Typedescriptor_t&, JSON_Tokenizer&, boolean);
};

struct Enum_Type : public Base_Type {
  virtual int as_int() const = 0;
  virtual void from_int(int) = 0;
};

extern boolean operator==(null_type null_value, const Empty_Record_Type& other_value);
extern boolean operator!=(null_type null_value, const Empty_Record_Type& other_value);

#undef VIRTUAL_IF_RUNTIME_2
#endif

/** Base class for values and templates with fuzzy or lazy evaluation
  *
  * When inheriting, override the virtual function eval_expr() to evaluate
  * the desired expression. Add new members and a new constructor if the
  * expression contains references to variables.
  * The class is also used by itself (without inheriting) to store the static
  * values/templates of fuzzy or lazy parameters (the ones that are known at
  * compile-time). */
template <class EXPR_TYPE>
class Lazy_Fuzzy_Expr {
protected:
  /** Evaluation type. True for fuzzy evaluation, false for lazy evaluation. */
  boolean fuzzy;
  /** Indicates whether a lazy expression has been evaluated. This flag is
    * updated in case of fuzzy evaluation, too, in case the evaluation type is
    * later changed to lazy. */
  boolean expr_evaluated;
  /** The previous value of expr_evaluated is stored here when it is reset.
    * (This is only needed if an evaluated lazy expression is changed to fuzzy,
    * then to lazy, and isn't evaluated until it is reverted back to the initial
    * lazy evaluation. Changing from fuzzy to lazy resets the evaluation flag,
    * and the information, that the initial lazy expression was evaluated, would
    * otherwise be lost.) */
  boolean old_expr_evaluated;
  /** Stores the value of the evaluated expression (in which case it is unbound
    * until the first evaluation) or a static value/template, if it is known at
    * compile-time. */
  EXPR_TYPE expr_cache;
  /** Evaluates the expression. Empty by default. Should be overridden by the
    * inheriting class. */
  virtual void eval_expr() {}
public:
  /** Constructor called by the inheriting class. */
  Lazy_Fuzzy_Expr(boolean p_fuzzy): fuzzy(p_fuzzy), expr_evaluated(FALSE), old_expr_evaluated(FALSE) {}
  /** Dummy type used by the static value/template constructor, since the
    * inheriting class' constructor may have parameters of any type. */
  enum evaluated_state_t { EXPR_EVALED };
  /** Constructor for static values and templates. */
  Lazy_Fuzzy_Expr(boolean p_fuzzy, evaluated_state_t /*p_es*/, EXPR_TYPE p_cache)
  : fuzzy(p_fuzzy), expr_evaluated(TRUE), old_expr_evaluated(TRUE), expr_cache(p_cache) {}
  /** Copy constructor (can't set it to private, since it is currently used by
    * the generated default altstep classes). */
  Lazy_Fuzzy_Expr(const Lazy_Fuzzy_Expr&) {
    TTCN_error("Internal error: Copying a fuzzy or lazy parameter.");
  }
  /** Casting operator. This function is called whenever the fuzzy or lazy
    * expression is referenced. */
  operator EXPR_TYPE&() {
    if (fuzzy || !expr_evaluated) {
      eval_expr();
      expr_evaluated = TRUE;
    }
    return expr_cache;
  }
  /** Virtual destructor. */
  virtual ~Lazy_Fuzzy_Expr() {}
  /** Logging function. Used by the TTCN-3 Debugger for printing the values of
    * lazy parameters. */
  void log() const {
    if (!expr_evaluated) {
      TTCN_Logger::log_event_str("<not evaluated>");
    }
    else {
      expr_cache.log();
    }
  }
  /** Changes the evaluation type (from lazy to fuzzy or from fuzzy to lazy). */
  void change() {
    fuzzy = !fuzzy;
    if (!fuzzy) {
      old_expr_evaluated = expr_evaluated;
      expr_evaluated = FALSE;
    }
  }
  /** Reverts the evaluation type back to its previous state. */
  void revert() {
    fuzzy = !fuzzy;
    if (fuzzy) {
      expr_evaluated = expr_evaluated || old_expr_evaluated;
    }
  }
};

/** Interface/base class for decoded content matching 
  *
  * For every decmatch template the compiler generates a new class that inherits
  * this one and implements its virtual functions. An instance of the new class
  * is stored in the template object, which calls the appropriate virtual
  * functions when the template object's match() or log() functions are called. */
class Dec_Match_Interface {
public:
  virtual boolean match(TTCN_Buffer&) = 0;
  virtual void log() const = 0;
  /** this returns the decoding result of the last successfully matched value,
    * which may be used by value and parameter redirect classes for optimization
    * (so they don't have to decode the same value again)
    * the function returns a void pointer (since the decoding could result in a
    * value of any type), which is converted to the required type when used */
  virtual void* get_dec_res() const = 0;
  /** this returns the decoded type's descriptor, which may be used by value and
    * parameter redirect classes to determine whether the redirected value would
    * be decoded into the same type as the type used in this decmatch template */
  virtual const TTCN_Typedescriptor_t* get_type_descr() const = 0;
  virtual ~Dec_Match_Interface() {}
};

/** Interface/base class for value redirects in RT2
  *
  * For every value redirect the compiler generates a new class that inherits
  * this one and implements its virtual function. An instance of the new class
  * is passed to a 'receive', 'getreply', 'catch' or 'done' function call as
  * parameter, which, if successful, calls the instance's set_values() function. */
#ifdef TITAN_RUNTIME_2
class Value_Redirect_Interface {
public:
  virtual void set_values(const Base_Type*) = 0;
  virtual ~Value_Redirect_Interface() { }
};
#endif

/** Base class for index redirects
  *
  * For every index redirect the compiler generates a new class that inherits
  * this one and implements its virtual function. An instance of the new class
  * is passed to a port, timer or component operation, performed on an array with
  * the help of the 'any from' clause, which, if successful, calls the instance's
  * add_index() function once for each dimension in the array.
  *
  * List of operations in question: receive, check-receive, trigger, getcall,
  * check-getcall, getreply, check-getreply, catch, check-catch, check, timeout,
  * running (for both components and timers), done, killed and alive. */
class Index_Redirect {
protected:
  /** If the port, timer or component operation in question succeeds, then the
    * index in the current dimension of the port, timer or component array is
    * stored in the array/record of element indicated by this member. Only used
    * if the indices are being redirected to an integer array or record of
    * integer. If the index is redirected to a single integer, then this member
    * is ignored. */
  int pos;
  
public:
  Index_Redirect() : pos(-1) { }
  virtual ~Index_Redirect() { }
  void incr_pos() { ++pos; }
  void decr_pos() { --pos; }
  virtual void add_index(int p_index) = 0;
};

#endif
