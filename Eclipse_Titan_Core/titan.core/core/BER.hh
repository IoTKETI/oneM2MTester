/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Forstner, Matyas
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef _BER_HH
#define _BER_HH

/** @name BER encoding subtypes
    @{
*/
/// Canonical Encoding Rules
#define BER_ENCODE_CER 1
/// Distinguished Encoding Rules
#define BER_ENCODE_DER 2
/** @} */

/** @name BER decoding options
    @{
*/
/// Short length form is acceptable
#define BER_ACCEPT_SHORT 0x01
/// Long length form is acceptable
#define BER_ACCEPT_LONG 0x02
/// Indefinite form is acceptable
#define BER_ACCEPT_INDEFINITE 0x04
/// Definite forms (short or long) are acceptable
#define BER_ACCEPT_DEFINITE 0x03
/// All forms are acceptable
#define BER_ACCEPT_ALL 0x07
/** @} */

#include "Types.h"
#include "Encdec.hh"

/**
  * \defgroup BER BER-related stuff.
  *
  *  @{
  */

/** @brief ASN.1 tag */
enum ASN_Tagclass_t {
  ASN_TAG_UNDEF,
  ASN_TAG_UNIV, /**< UNIVERSAL */
  ASN_TAG_APPL, /**< APPLICATION */
  ASN_TAG_CONT, /**< context-specific */
  ASN_TAG_PRIV  /**< PRIVATE */
};

typedef unsigned int ASN_Tagnumber_t;

/** @brief ASN.1 identifier
  *
  * Contains two thirds of the T from a TLV
  */
struct ASN_Tag_t {
  ASN_Tagclass_t tagclass; /**< Tag class */
  ASN_Tagnumber_t tagnumber; /**< Tag value. For UNIVERSAL, the values are predefined */

  /** Creates a user-friendly representation of the tag. After using,
    * you have to Free() this string. */
  char* print() const;
};

/** @brief BER encoding information
 *
 * There is one object for each predefined (UNIVERSAL) type.
 * Also, the compiler writes an object of this type for each user-defined type
 * into the generated code in Common::Type::generate_code_berdescriptor() .
 */
struct ASN_BERdescriptor_t {
  /** Number of tags.
   *
   *  For the UNIVERSAL classes, this is usually 1 (except for CHOICE and ANY)
   */
  size_t n_tags;
  /** This array contains the tags. Index 0 is the innermost tag. */
  const ASN_Tag_t *tags;

  /** Creates a user-friendly representation of tags. After using, you
    * have to Free() this string. */
  char* print_tags() const;
};

struct TTCN_Typedescriptor_t;

extern const ASN_BERdescriptor_t BOOLEAN_ber_;
extern const ASN_BERdescriptor_t INTEGER_ber_;
extern const ASN_BERdescriptor_t BITSTRING_ber_;
extern const ASN_BERdescriptor_t OCTETSTRING_ber_;
extern const ASN_BERdescriptor_t ASN_NULL_ber_;
extern const ASN_BERdescriptor_t OBJID_ber_;
extern const ASN_BERdescriptor_t ObjectDescriptor_ber_;
extern const ASN_BERdescriptor_t FLOAT_ber_;
extern const ASN_BERdescriptor_t ENUMERATED_ber_;
extern const ASN_BERdescriptor_t UTF8String_ber_;
extern const ASN_BERdescriptor_t ASN_ROID_ber_;
extern const ASN_BERdescriptor_t SEQUENCE_ber_;
extern const ASN_BERdescriptor_t SET_ber_;
extern const ASN_BERdescriptor_t CHOICE_ber_;
extern const ASN_BERdescriptor_t ASN_ANY_ber_;
extern const ASN_BERdescriptor_t NumericString_ber_;
extern const ASN_BERdescriptor_t PrintableString_ber_;
extern const ASN_BERdescriptor_t TeletexString_ber_;
extern const ASN_BERdescriptor_t VideotexString_ber_;
extern const ASN_BERdescriptor_t IA5String_ber_;
extern const ASN_BERdescriptor_t ASN_UTCTime_ber_;
extern const ASN_BERdescriptor_t ASN_GeneralizedTime_ber_;
extern const ASN_BERdescriptor_t GraphicString_ber_;
extern const ASN_BERdescriptor_t VisibleString_ber_;
extern const ASN_BERdescriptor_t GeneralString_ber_;
extern const ASN_BERdescriptor_t UniversalString_ber_;
extern const ASN_BERdescriptor_t BMPString_ber_;
extern const ASN_BERdescriptor_t EXTERNAL_ber_;
extern const ASN_BERdescriptor_t EMBEDDED_PDV_ber_;
extern const ASN_BERdescriptor_t CHARACTER_STRING_ber_;

/** An unsigned long long with 7 bits set (0x7F) in the most significant byte.
 *  Used when decoding objid values. */
extern const unsigned long long unsigned_llong_7msb;

/** An unsigned long with 8 bits set (0xFF) in the most significant byte.
 *  Used when decoding integers. */
extern const unsigned long unsigned_long_8msb;

/** How many bits are needed minimally to represent the given
  * (nonnegative integral) value. Returned value is greater than or
  * equal to 1. */
template<typename T>
size_t min_needed_bits(T p)
{
  if(p==0) return 1;
  register size_t n=0;
  register T tmp=p;
  while(tmp!=0) n++, tmp/=2;
  return n;
}

/** This struct represents a BER TLV.
 *
 * It represents a "deconstructed" (cooked) representation of
 * a BER encoding. */
struct ASN_BER_TLV_t {
  boolean isConstructed;   /**< Primitive/Constructed bit. */
  boolean V_tlvs_selected; /**< How the data is stored in \p V. */
  boolean isLenDefinite;   /**< \c false for indefinite form */
  boolean isLenShort;      /**< \c true for short form (0-127) */
  /** Is \p tagclass and \p tagnumber valid.
   *  ASN_BER_str2TLV function sets this. */
  boolean isTagComplete;
  boolean isComplete;
  ASN_Tagclass_t tagclass; /**< UNIV/APP/context/PRIVATE */
  ASN_Tagnumber_t tagnumber;
  size_t Tlen; /** Length of T part. May be >1 for tag values >30 */
  size_t Llen; /** Length of L part. */
  unsigned char *Tstr; /** Encoded T part. */
  unsigned char *Lstr; /** Encoded L part. */
  union { /* depends on V_tlvs_selected */
    struct {
      size_t Vlen;
      unsigned char *Vstr;
    } str; /**< V_tlvs_selected==FALSE */
    struct {
      size_t n_tlvs;
      ASN_BER_TLV_t **tlvs;
    } tlvs; /**< V_tlvs_selected==TRUE */
  } V;

  /** Creates a new constructed TLV which contains one TLV (\a
    * p_tlv). Or, if the parameter is NULL, then an empty constructed
    * TLV is created. */
  static ASN_BER_TLV_t* construct(ASN_BER_TLV_t *p_tlv);
  /** Creates a new non-constructed TLV with the given V-part.
    * Tagclass and -number are set to UNIV-0. If \a p_Vstr is NULL
    * then allocates a \a p_Vlen size buffer for Vstr.
    * Takes over ownership of \a p_Vstr */
  static ASN_BER_TLV_t* construct(size_t p_Vlen, unsigned char *p_Vstr);
  /** Creates a new non-constructed TLV with empty T, L and V part. */
  static ASN_BER_TLV_t* construct();
  /** Frees the given \a p_tlv recursively. Tstr, Lstr and
    * Vstr/tlvs. If \a no_str is TRUE, then only the tlvs are deleted,
    * the strings (Tstr, Lstr, Vstr) are not. This is useful during
    * decoding, when the Xstr points into a buffer.*/
  static void destruct(ASN_BER_TLV_t *p_tlv, boolean no_str=FALSE);
  /** Checks the 'constructed' flag. If fails, raises an EncDec
    * error. */
  void chk_constructed_flag(boolean flag_expected) const;
  /** Adds \a p_tlv to this (constructed) TLV.
   * @pre isConstructed   is \c true
   * @pre V_tlvs_selected is \c true */
  void add_TLV(ASN_BER_TLV_t *p_tlv);
  /** Adds a new UNIVERSAL 0 TLV to this (constructed) TLV. */
  void add_UNIV0_TLV();
  /** Returns the length (in bytes) of the full TLV. */
  size_t get_len() const;
  /** Adds T and L part to TLV. It adds trailing UNI-0 TLV (two zero
    * octets) if the length form is indefinite. Precondition:
    * isConstructed (and V_tlvs_selected) and V-part are valid. */
  void add_TL(ASN_Tagclass_t p_tagclass,
              ASN_Tagnumber_t p_tagnumber,
              unsigned coding);
  /** Puts the TLV in buffer \a p_buf. */
  void put_in_buffer(TTCN_Buffer& p_buf);
  /** Gets the octet in position \a pos. Works also for constructed TLVs. */
  unsigned char get_pos(size_t pos) const;
private:
  unsigned char _get_pos(size_t& pos, boolean& success) const;
public:
  /** Compares the TLVs as described in X.690 11.6. Returns -1 if this
    * less than, 0 if equal to, 1 if greater than other. */
  int compare(const ASN_BER_TLV_t *other) const;
  /** Sorts its TLVs according to their octetstring representation, as
    * described in X.690 11.6. */
  void sort_tlvs();
  /** Compares the TLVs according their tags as described in X.680
    * 8.6. Returns -1 if this less than, 0 if equal to, 1 if greater
    * than other. */
  int compare_tags(const ASN_BER_TLV_t *other) const;
  /** Sorts its TLVs according to their tags, as described in X.690
    * 10.3. */
  void sort_tlvs_tag();
};

/**
  * Searches the T, L and V parts of the TLV represented in \a s, and
  * stores it in \a tlv. Returns FALSE if there isn't a complete
  * TLV-structure in \a s, TRUE otherwise.
  */
boolean ASN_BER_str2TLV(size_t p_len_s,
                        const unsigned char* p_str,
                        ASN_BER_TLV_t& tlv,
                        unsigned L_form);

/**
  * Adds the T and L part of the TLV. If there are multiple tags, then
  * adds them all. Precondition: the following fields are ready of \a
  * p_tlv: isConstructed, V_tlvs_selected, isLenDefinite, isLenShort
  * and V. If tagclass and tagnumber are UNIVERSAL 0, then replaces
  * them, otherwise 'wraps' p_tlv in a new TLV.
  */
ASN_BER_TLV_t* ASN_BER_V2TLV(ASN_BER_TLV_t* p_tlv,
                             const TTCN_Typedescriptor_t& p_td,
                             unsigned coding);

/** @} end of BER group */

#endif // _BER_HH
