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
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *
 ******************************************************************************/
#include <string.h>

#include "ASN_External.hh"

#include "ASN_Any.hh"
#include "ASN_Null.hh"
#include "Bitstring.hh"
#include "Integer.hh"
#include "Objid.hh"
#include "Octetstring.hh"
#include "Universal_charstring.hh"

#include "Parameters.h"
#include "Param_Types.hh"
#include "Error.hh"
#include "Logger.hh"
#include "Encdec.hh"
#include "BER.hh"
#include "Addfunc.hh"

#include "../common/dbgnew.hh"

/*
 * This type is used to BER encode/decode the EXTERNAL type.
 * For details, see X.690 8.18.
 */

/*

to do when regenerating:

in .hh file:

add __SUNPRO_CC ifdefs for single_value_struct

delete encode/decode members except for EXTERNAL

in .cc file:

leave transfer syntax in anonymous namespace

delete encode/decode members except for EXTERNAL
leave EXTERNAL::BER_encode_TLV() -- written by hand
leave EXTERNAL::BER_decode_TLV() -- written by hand

replace '@EXTERNAL' with 'EXTERNAL'

remove RAW and TEXT enc/dec functions

*/

namespace { /* anonymous namespace */

  class EXTERNALtransfer_encoding;
  class EXTERNALtransfer;

  class EXTERNALtransfer_encoding : public Base_Type {
  public:
    enum union_selection_type { UNBOUND_VALUE = 0, ALT_single__ASN1__type = 1, ALT_octet__aligned = 2, ALT_arbitrary = 3 };
  private:
    union_selection_type union_selection;
    union {
      ASN_ANY *field_single__ASN1__type;
      OCTETSTRING *field_octet__aligned;
      BITSTRING *field_arbitrary;
    };
    void clean_up();
    void copy_value(const EXTERNALtransfer_encoding& other_value);

  public:
    EXTERNALtransfer_encoding()
    { union_selection = UNBOUND_VALUE; }
    EXTERNALtransfer_encoding(const EXTERNALtransfer_encoding& other_value)
    : Base_Type(other_value)
    { copy_value(other_value); }
    ~EXTERNALtransfer_encoding() { clean_up(); }
    EXTERNALtransfer_encoding& operator=(const EXTERNALtransfer_encoding& other_value);
    ASN_ANY& single__ASN1__type();
    const ASN_ANY& single__ASN1__type() const;
    OCTETSTRING& octet__aligned();
    const OCTETSTRING& octet__aligned() const;
    BITSTRING& arbitrary();
    const BITSTRING& arbitrary() const;
    inline union_selection_type get_selection() const { return union_selection; }
#ifdef TITAN_RUNTIME_2
    void set_param(Module_Param& /*param*/) { TTCN_error("Internal error: EXTERNALtransfer_encoding::set_param() called."); }
    Module_Param* get_param(Module_Param_Name& /*param_name*/) const { TTCN_error("Internal error: EXTERNALtransfer_encoding::get_param() called."); }
    void encode_text(Text_Buf& /*text_buf*/) const { TTCN_error("Internal error: EXTERNALtransfer_encoding::encode_text() called."); }
    void decode_text(Text_Buf& /*text_buf*/) { TTCN_error("Internal error: EXTERNALtransfer_encoding::decode_text() called."); }
    boolean is_bound() const { return union_selection!=UNBOUND_VALUE; }
    boolean is_equal(const Base_Type* /*other_value*/) const { TTCN_error("Internal error: EXTERNALtransfer_encoding::is_equal() called."); }
    void set_value(const Base_Type* /*other_value*/) { TTCN_error("Internal error: EXTERNALtransfer_encoding::set_value() called."); }
    Base_Type* clone() const { TTCN_error("Internal error: EXTERNALtransfer_encoding::clone() called."); }
    const TTCN_Typedescriptor_t* get_descriptor() const { TTCN_error("Internal error: EXTERNALtransfer_encoding::get_descriptor() called."); }
#endif
    ASN_BER_TLV_t* BER_encode_TLV(const TTCN_Typedescriptor_t& p_td, unsigned p_coding) const;
    boolean BER_decode_TLV(const TTCN_Typedescriptor_t& p_td, const ASN_BER_TLV_t& p_tlv, unsigned L_form);
    int XER_encode(const XERdescriptor_t& p_td,
                   TTCN_Buffer& p_buf, unsigned int flavor, unsigned int flavor2, int indent, embed_values_enc_struct_t*) const;
    int XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& reader,
                   unsigned int flavor, unsigned int flavor2, embed_values_dec_struct_t*);
  private:
    boolean BER_decode_set_selection(const ASN_BER_TLV_t& p_tlv);
  public:
    boolean BER_decode_isMyMsg(const TTCN_Typedescriptor_t& p_td, const ASN_BER_TLV_t& p_tlv);
  };

  /** This class is used to encode/decode the EXTERNAL type.
   *
   * The sequence type used for encoding/decoding the EXTERNAL type
   * differs from the associated type of EXTERNAL. See
   *   = X.690 (BER), 8.18.1 ;
   *   = X.691 (PER), 26.1 ;
   *   = X.693 (XER), 8.4
   *
   * The actual encoding/decoding is performed by this class and its members.
   * Data is transferred to/from an object of class EXTERNAL. */
  class EXTERNALtransfer : public Base_Type {
    OPTIONAL<OBJID> field_direct__reference;
    OPTIONAL<INTEGER> field_indirect__reference;
    OPTIONAL<ObjectDescriptor> field_data__value__descriptor;
    EXTERNALtransfer_encoding field_encoding;
  public:
    void load(const EXTERNAL& ex);
    inline OPTIONAL<OBJID>& direct__reference()
    {return field_direct__reference;}
    inline const OPTIONAL<OBJID>& direct__reference() const
    {return field_direct__reference;}
    inline OPTIONAL<INTEGER>& indirect__reference()
    {return field_indirect__reference;}
    inline const OPTIONAL<INTEGER>& indirect__reference() const
    {return field_indirect__reference;}
    inline OPTIONAL<ObjectDescriptor>& data__value__descriptor()
    {return field_data__value__descriptor;}
    inline const OPTIONAL<ObjectDescriptor>& data__value__descriptor() const
    {return field_data__value__descriptor;}
    inline EXTERNALtransfer_encoding& encoding()
    {return field_encoding;}
    inline const EXTERNALtransfer_encoding& encoding() const
    {return field_encoding;}
#ifdef TITAN_RUNTIME_2
    void set_param(Module_Param& /*param*/) { TTCN_error("Internal error: EXTERNALtransfer::set_param() called."); }
    Module_Param* get_param(Module_Param_Name& /*param_name*/) const { TTCN_error("Internal error: EXTERNALtransfer::get_param() called."); }
    void encode_text(Text_Buf& /*text_buf*/) const { TTCN_error("Internal error: EXTERNALtransfer::encode_text() called."); }
    void decode_text(Text_Buf& /*text_buf*/) { TTCN_error("Internal error: EXTERNALtransfer::decode_text() called."); }
    boolean is_bound() const { TTCN_error("Internal error: EXTERNALtransfer::is_bound() called."); }
    boolean is_value() const { TTCN_error("Internal error: EXTERNALtransfer::is_value() called."); }
    void clean_up() { TTCN_error("Internal error: EXTERNALtransfer::clean_up() called."); }
    boolean is_equal(const Base_Type* /*other_value*/) const { TTCN_error("Internal error: EXTERNALtransfer::is_equal() called."); }
    void set_value(const Base_Type* /*other_value*/) { TTCN_error("Internal error: EXTERNALtransfer::set_value() called."); }
    Base_Type* clone() const { TTCN_error("Internal error: EXTERNALtransfer::clone() called."); }
    const TTCN_Typedescriptor_t* get_descriptor() const { TTCN_error("Internal error: EXTERNALtransfer::get_descriptor() called."); }
#endif
    ASN_BER_TLV_t* BER_encode_TLV(const TTCN_Typedescriptor_t& p_td, unsigned p_coding) const;
    boolean BER_decode_TLV(const TTCN_Typedescriptor_t& p_td, const ASN_BER_TLV_t& p_tlv, unsigned L_form);
    int XER_encode(const XERdescriptor_t& p_td,
                   TTCN_Buffer& p_buf, unsigned int flavor, unsigned int flavor2, int indent, embed_values_enc_struct_t*) const;
    int XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& reader,
                   unsigned int flavor, unsigned int flavor2, embed_values_dec_struct_t*);
  };

  /** Transform the information from the visible format to the encoding format
   *
   * Called from EXTERNAL::XER_encode() */
  void EXTERNALtransfer::load(const EXTERNAL& ex)
  {
    // ALT_syntaxes, ALT_transfer__syntax and ALT_fixed do not appear below.
    // These are forbidden for the EXTERNAL type.
    switch(ex.identification().get_selection()) {
    case EXTERNAL_identification::ALT_syntax:
      field_direct__reference=ex.identification().syntax();
      break;
    case EXTERNAL_identification::ALT_context__negotiation:
	field_direct__reference=ex.identification().context__negotiation().transfer__syntax();
      break;
    default:
	field_direct__reference=OMIT_VALUE;
	break;
    }
    switch(ex.identification().get_selection()) {
    case EXTERNAL_identification::ALT_presentation__context__id:
	field_indirect__reference=ex.identification().presentation__context__id();
      break;
    case EXTERNAL_identification::ALT_context__negotiation:
	field_indirect__reference=ex.identification().context__negotiation().presentation__context__id();
      break;
    default:
	field_indirect__reference=OMIT_VALUE;
	break;
    }
    field_data__value__descriptor=ex.data__value__descriptor();
    field_encoding.octet__aligned()=ex.data__value();
  }

  static const TTCN_Typedescriptor_t EXTERNALtransfer_encoding_descr_ = { "EXTERNALtransfer.encoding", &CHOICE_ber_, NULL, NULL, NULL, NULL, NULL, TTCN_Typedescriptor_t::DONTCARE };

  static const ASN_Tag_t EXTERNALtransfer_encoding_single__ASN1__type_tag_[] = { { ASN_TAG_CONT, 0u } };
  static const ASN_BERdescriptor_t EXTERNALtransfer_encoding_single__ASN1__type_ber_ = { 1u, EXTERNALtransfer_encoding_single__ASN1__type_tag_ };
  static const TTCN_Typedescriptor_t EXTERNALtransfer_encoding_single__ASN1__type_descr_ = { "EXTERNALtransfer.encoding.single-ASN1-type", &EXTERNALtransfer_encoding_single__ASN1__type_ber_, NULL, NULL, NULL, NULL, NULL, TTCN_Typedescriptor_t::DONTCARE };

  static const ASN_Tag_t EXTERNALtransfer_encoding_octet__aligned_tag_[] = { { ASN_TAG_CONT, 1u } };
  static const ASN_BERdescriptor_t EXTERNALtransfer_encoding_octet__aligned_ber_ = { 1u, EXTERNALtransfer_encoding_octet__aligned_tag_ };
  static const TTCN_Typedescriptor_t EXTERNALtransfer_encoding_octet__aligned_descr_ = { "EXTERNALtransfer.encoding.octet-aligned", &EXTERNALtransfer_encoding_octet__aligned_ber_, NULL, NULL, NULL, NULL, NULL, TTCN_Typedescriptor_t::DONTCARE };

  static const ASN_Tag_t EXTERNALtransfer_encoding_arbitrary_tag_[] = { { ASN_TAG_CONT, 2u } };
  static const ASN_BERdescriptor_t EXTERNALtransfer_encoding_arbitrary_ber_ = { 1u, EXTERNALtransfer_encoding_arbitrary_tag_ };
  static const TTCN_Typedescriptor_t EXTERNALtransfer_encoding_arbitrary_descr_ = { "EXTERNALtransfer.encoding.arbitrary", &EXTERNALtransfer_encoding_arbitrary_ber_, NULL, NULL, NULL, NULL, NULL, TTCN_Typedescriptor_t::DONTCARE };

  /* Member functions of C++ classes */

  void EXTERNALtransfer_encoding::clean_up()
  {
    switch (union_selection) {
    case ALT_single__ASN1__type:
      delete field_single__ASN1__type;
      break;
    case ALT_octet__aligned:
      delete field_octet__aligned;
      break;
    case ALT_arbitrary:
      delete field_arbitrary;
      break;
    default:
      break;
    }
    union_selection = UNBOUND_VALUE;
  }

  void EXTERNALtransfer_encoding::copy_value(const EXTERNALtransfer_encoding& other_value)
  {
    switch (other_value.union_selection) {
    case ALT_single__ASN1__type:
      field_single__ASN1__type = new ASN_ANY(*other_value.field_single__ASN1__type);
      break;
    case ALT_octet__aligned:
      field_octet__aligned = new OCTETSTRING(*other_value.field_octet__aligned);
      break;
    case ALT_arbitrary:
      field_arbitrary = new BITSTRING(*other_value.field_arbitrary);
      break;
    default:
      TTCN_error("Assignment of an unbound union value of type EXTERNALtransfer.encoding.");
    }
    union_selection = other_value.union_selection;
  }

  EXTERNALtransfer_encoding& EXTERNALtransfer_encoding::operator=(const EXTERNALtransfer_encoding& other_value)
  {
    if(this != &other_value) {
      clean_up();
      copy_value(other_value);
    }
    return *this;
  }

  ASN_ANY& EXTERNALtransfer_encoding::single__ASN1__type()
  {
    if (union_selection != ALT_single__ASN1__type) {
      clean_up();
      field_single__ASN1__type = new ASN_ANY;
      union_selection = ALT_single__ASN1__type;
    }
    return *field_single__ASN1__type;
  }

  const ASN_ANY& EXTERNALtransfer_encoding::single__ASN1__type() const
  {
    if (union_selection != ALT_single__ASN1__type) TTCN_error("Using non-selected field single-ASN1-type in a value of union type EXTERNALtransfer.encoding.");
    return *field_single__ASN1__type;
  }

  OCTETSTRING& EXTERNALtransfer_encoding::octet__aligned()
  {
    if (union_selection != ALT_octet__aligned) {
      clean_up();
      field_octet__aligned = new OCTETSTRING;
      union_selection = ALT_octet__aligned;
    }
    return *field_octet__aligned;
  }

  const OCTETSTRING& EXTERNALtransfer_encoding::octet__aligned() const
  {
    if (union_selection != ALT_octet__aligned) TTCN_error("Using non-selected field octet-aligned in a value of union type EXTERNALtransfer.encoding.");
    return *field_octet__aligned;
  }

  BITSTRING& EXTERNALtransfer_encoding::arbitrary()
  {
    if (union_selection != ALT_arbitrary) {
      clean_up();
      field_arbitrary = new BITSTRING;
      union_selection = ALT_arbitrary;
    }
    return *field_arbitrary;
  }

  const BITSTRING& EXTERNALtransfer_encoding::arbitrary() const
  {
    if (union_selection != ALT_arbitrary) TTCN_error("Using non-selected field arbitrary in a value of union type EXTERNALtransfer.encoding.");
    return *field_arbitrary;
  }

  ASN_BER_TLV_t* EXTERNALtransfer_encoding::BER_encode_TLV(const TTCN_Typedescriptor_t& p_td, unsigned p_coding) const
  {
    BER_chk_descr(p_td);
    ASN_BER_TLV_t *new_tlv;
    TTCN_EncDec_ErrorContext ec_0("Alternative '");
    TTCN_EncDec_ErrorContext ec_1;
    switch (union_selection) {
    case ALT_single__ASN1__type:
      ec_1.set_msg("single-ASN1-type': ");
      new_tlv=field_single__ASN1__type->BER_encode_TLV(EXTERNALtransfer_encoding_single__ASN1__type_descr_, p_coding);
      break;
    case ALT_octet__aligned:
      ec_1.set_msg("octet-aligned': ");
      new_tlv=field_octet__aligned->BER_encode_TLV(EXTERNALtransfer_encoding_octet__aligned_descr_, p_coding);
      break;
    case ALT_arbitrary:
      ec_1.set_msg("arbitrary': ");
      new_tlv=field_arbitrary->BER_encode_TLV(EXTERNALtransfer_encoding_arbitrary_descr_, p_coding);
      break;
    case UNBOUND_VALUE:
      new_tlv=BER_encode_chk_bound(FALSE);
      break;
    default:
      TTCN_EncDec_ErrorContext::error_internal("Unknown selection.");
      new_tlv = NULL;
      break;
    }
    return ASN_BER_V2TLV(new_tlv, p_td, p_coding);
  }

  boolean EXTERNALtransfer_encoding::BER_decode_set_selection(const ASN_BER_TLV_t& p_tlv)
  {
    clean_up();
    union_selection=ALT_single__ASN1__type;
    field_single__ASN1__type=new ASN_ANY;
    if(field_single__ASN1__type->BER_decode_isMyMsg(EXTERNALtransfer_encoding_single__ASN1__type_descr_, p_tlv))
      return TRUE;
    delete field_single__ASN1__type;
    union_selection=ALT_octet__aligned;
    field_octet__aligned=new OCTETSTRING;
    if(field_octet__aligned->BER_decode_isMyMsg(EXTERNALtransfer_encoding_octet__aligned_descr_, p_tlv))
      return TRUE;
    delete field_octet__aligned;
    union_selection=ALT_arbitrary;
    field_arbitrary=new BITSTRING;
    if(field_arbitrary->BER_decode_isMyMsg(EXTERNALtransfer_encoding_arbitrary_descr_, p_tlv))
      return TRUE;
    delete field_arbitrary;
    union_selection=UNBOUND_VALUE;
    return FALSE;
  }

  boolean EXTERNALtransfer_encoding::BER_decode_isMyMsg(const TTCN_Typedescriptor_t& p_td, const ASN_BER_TLV_t& p_tlv)
  {
    if(p_td.ber->n_tags==0) {
      EXTERNALtransfer_encoding tmp_type;
      return tmp_type.BER_decode_set_selection(p_tlv);
    }
    else return Base_Type::BER_decode_isMyMsg(p_td, p_tlv);
  }

  boolean EXTERNALtransfer_encoding::BER_decode_TLV(const TTCN_Typedescriptor_t& p_td, const ASN_BER_TLV_t& p_tlv, unsigned L_form)
  {
    BER_chk_descr(p_td);
    ASN_BER_TLV_t stripped_tlv;
    BER_decode_strip_tags(*p_td.ber, p_tlv, L_form, stripped_tlv);
    TTCN_EncDec_ErrorContext ec_0("While decoding 'EXTERNALtransfer.encoding' type: ");
    ASN_BER_TLV_t tmp_tlv;
    if(!BER_decode_TLV_CHOICE(*p_td.ber, stripped_tlv, L_form, tmp_tlv) || !BER_decode_CHOICE_selection(BER_decode_set_selection(tmp_tlv), tmp_tlv))
      return FALSE;
    TTCN_EncDec_ErrorContext ec_1("Alternative '");
    TTCN_EncDec_ErrorContext ec_2;
    switch (union_selection) {
    case ALT_single__ASN1__type:
      ec_2.set_msg("single-ASN1-type': ");
      field_single__ASN1__type->BER_decode_TLV(EXTERNALtransfer_encoding_single__ASN1__type_descr_, tmp_tlv, L_form);
      break;
    case ALT_octet__aligned:
      ec_2.set_msg("octet-aligned': ");
      field_octet__aligned->BER_decode_TLV(EXTERNALtransfer_encoding_octet__aligned_descr_, tmp_tlv, L_form);
      break;
    case ALT_arbitrary:
      ec_2.set_msg("arbitrary': ");
      field_arbitrary->BER_decode_TLV(EXTERNALtransfer_encoding_arbitrary_descr_, tmp_tlv, L_form);
      break;
    default:
      return FALSE;
    }
    return TRUE;
  }

  int EXTERNALtransfer_encoding::XER_encode(const XERdescriptor_t& p_td,
    TTCN_Buffer& p_buf, unsigned int flavor, unsigned int flavor2, int indent, embed_values_enc_struct_t*) const
  {
    int indenting = !is_canonical(flavor);
    boolean exer  = is_exer(flavor);
    int encoded_length=(int)p_buf.get_len();
    if (indenting) do_indent(p_buf, indent);
    p_buf.put_c('<');
    if (exer) write_ns_prefix(p_td, p_buf);
    p_buf.put_s((size_t)p_td.namelens[exer] - 1 + indenting, (const unsigned char*)p_td.names[exer]);

    ++indent;
    switch (union_selection) {
    case ALT_single__ASN1__type:
      field_single__ASN1__type->XER_encode(EXTERNAL_encoding_singleASN_xer_, p_buf, flavor, flavor2, indent, 0);
      break;
    case ALT_octet__aligned:
      field_octet__aligned    ->XER_encode(EXTERNAL_encoding_octet_aligned_xer_, p_buf, flavor, flavor2, indent, 0);
      break;
    case ALT_arbitrary:
      field_arbitrary         ->XER_encode(EXTERNAL_encoding_arbitrary_xer_, p_buf, flavor, flavor2, indent, 0);
      break;
    case UNBOUND_VALUE:
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
        "Encoding an unbound value");
      break;
    default:
      TTCN_EncDec_ErrorContext::error_internal("Unknown selection.");
      // TODO something at all ?
      break;
    }

    if (indenting) do_indent(p_buf, --indent);
    p_buf.put_c('<');
    p_buf.put_c('/');
    if (exer) write_ns_prefix(p_td, p_buf);
    p_buf.put_s((size_t)p_td.namelens[exer] - 1 + indenting, (const unsigned char*)p_td.names[exer]);
    return (int)p_buf.get_len() - encoded_length;
  }

  int EXTERNALtransfer_encoding::XER_decode(const XERdescriptor_t& p_td,
    XmlReaderWrap& reader, unsigned int flavor, unsigned int flavor2, embed_values_dec_struct_t*)
  {
    boolean exer  = is_exer(flavor);
    int  success = reader.Ok(), type, depth = -1;
    for (; success==1; success = reader.Read()) {
      type = reader.NodeType();
      if (type == XML_READER_TYPE_ELEMENT) {
	verify_name(reader, p_td, exer);
	depth = reader.Depth();
	break;
      }
    } // next

    const char * name = 0;
    for (success = reader.Read(); success == 1; success = reader.Read()) {
      type = reader.NodeType();
      if (XML_READER_TYPE_ELEMENT == type) break;
      else if (XML_READER_TYPE_END_ELEMENT == type) goto bail;
    }
    name = (const char*)reader.Name();

    switch (*name) {
    case 's': // single-ASN1-type
      single__ASN1__type().XER_decode(EXTERNAL_encoding_singleASN_xer_, reader, flavor, flavor2, 0);
      break;

    case 'o': // octet-aligned
      octet__aligned().XER_decode(EXTERNAL_encoding_octet_aligned_xer_, reader, flavor, flavor2, 0);
      break;

    case 'a': // arbitrary
      arbitrary().XER_decode(EXTERNAL_encoding_arbitrary_xer_, reader, flavor, flavor2, 0);
      break;

    default:
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG,
	"Doh!"); // FIXME error method and text
      break;
    }

    for (success = reader.Read(); success==1; success = reader.Read()) {
      type = reader.NodeType();
      if (XML_READER_TYPE_END_ELEMENT == type) {
	verify_end(reader, p_td, depth, exer);
	reader.Read(); // one last time
	break;
      }
    }
    bail:
    return 0; // FIXME return value
  }

  /******************** EXTERNALtransfer class ********************/

  ASN_BER_TLV_t* EXTERNALtransfer::BER_encode_TLV(const TTCN_Typedescriptor_t& p_td, unsigned p_coding) const
  {
    BER_chk_descr(p_td);
    ASN_BER_TLV_t *new_tlv=ASN_BER_TLV_t::construct(NULL);
    TTCN_EncDec_ErrorContext ec_0("Component '");
    TTCN_EncDec_ErrorContext ec_1;
    ec_1.set_msg("direct-reference': ");
    new_tlv->add_TLV(field_direct__reference.BER_encode_TLV(OBJID_descr_, p_coding));
    ec_1.set_msg("indirect-reference': ");
    new_tlv->add_TLV(field_indirect__reference.BER_encode_TLV(INTEGER_descr_, p_coding));
    ec_1.set_msg("data-value-descriptor': ");
    new_tlv->add_TLV(field_data__value__descriptor.BER_encode_TLV(ObjectDescriptor_descr_, p_coding));
    ec_1.set_msg("encoding': ");
    new_tlv->add_TLV(field_encoding.BER_encode_TLV(EXTERNALtransfer_encoding_descr_, p_coding));
    new_tlv=ASN_BER_V2TLV(new_tlv, p_td, p_coding);
    return new_tlv;
  }

  boolean EXTERNALtransfer::BER_decode_TLV(const TTCN_Typedescriptor_t& p_td, const ASN_BER_TLV_t& p_tlv, unsigned L_form)
  {
    BER_chk_descr(p_td);
    ASN_BER_TLV_t stripped_tlv;
    BER_decode_strip_tags(*p_td.ber, p_tlv, L_form, stripped_tlv);
    TTCN_EncDec_ErrorContext ec_0("While decoding 'EXTERNALtransfer' type: ");
    stripped_tlv.chk_constructed_flag(TRUE);
    size_t V_pos=0;
    ASN_BER_TLV_t tmp_tlv;
    boolean tlv_present=FALSE;
    {
      TTCN_EncDec_ErrorContext ec_1("Component '");
      TTCN_EncDec_ErrorContext ec_2;
      ec_2.set_msg("direct-reference': ");
      if(!tlv_present) tlv_present=BER_decode_constdTLV_next(stripped_tlv, V_pos, L_form, tmp_tlv);
      if(!tlv_present) field_direct__reference=OMIT_VALUE;
      else {
        field_direct__reference.BER_decode_TLV(OBJID_descr_, tmp_tlv, L_form);
        if(field_direct__reference.ispresent()) tlv_present=FALSE;
      }
      ec_2.set_msg("indirect-reference': ");
      if(!tlv_present) tlv_present=BER_decode_constdTLV_next(stripped_tlv, V_pos, L_form, tmp_tlv);
      if(!tlv_present) field_indirect__reference=OMIT_VALUE;
      else {
        field_indirect__reference.BER_decode_TLV(INTEGER_descr_, tmp_tlv, L_form);
        if(field_indirect__reference.ispresent()) tlv_present=FALSE;
      }
      ec_2.set_msg("data-value-descriptor': ");
      if(!tlv_present) tlv_present=BER_decode_constdTLV_next(stripped_tlv, V_pos, L_form, tmp_tlv);
      if(!tlv_present) field_data__value__descriptor=OMIT_VALUE;
      else {
        field_data__value__descriptor.BER_decode_TLV(ObjectDescriptor_descr_, tmp_tlv, L_form);
        if(field_data__value__descriptor.ispresent()) tlv_present=FALSE;
      }
      ec_2.set_msg("encoding': ");
      if(!tlv_present) tlv_present=BER_decode_constdTLV_next(stripped_tlv, V_pos, L_form, tmp_tlv);
      if(!tlv_present) return FALSE;
      field_encoding.BER_decode_TLV(EXTERNALtransfer_encoding_descr_, tmp_tlv, L_form);
      tlv_present=FALSE;
    }
    BER_decode_constdTLV_end(stripped_tlv, V_pos, L_form, tmp_tlv, tlv_present);
    return TRUE;
  }

  int EXTERNALtransfer::XER_encode(const XERdescriptor_t& p_td,
    TTCN_Buffer& p_buf, unsigned int flavor, unsigned int flavor2, int indent, embed_values_enc_struct_t*) const
  {
    int indenting = !is_canonical(flavor);
    boolean exer  = is_exer(flavor);
    int encoded_length=(int)p_buf.get_len();
    if (indenting) do_indent(p_buf, indent);
    p_buf.put_c('<');
    if (exer) write_ns_prefix(p_td, p_buf);
    p_buf.put_s((size_t)p_td.namelens[exer] - 1 + indenting, (const unsigned char*)p_td.names[exer]);

    ++indent;
    field_direct__reference      .XER_encode(EXTERNAL_direct_reference_xer_     , p_buf, flavor, flavor2, indent, 0);
    field_indirect__reference    .XER_encode(EXTERNAL_indirect_reference_xer_   , p_buf, flavor, flavor2, indent, 0);
    field_data__value__descriptor.XER_encode(EXTERNAL_data_value_descriptor_xer_, p_buf, flavor, flavor2, indent, 0);
    field_encoding               .XER_encode(EXTERNAL_encoding_xer_ , p_buf, flavor, flavor2, indent, 0);

    if (indenting) do_indent(p_buf, --indent);
    p_buf.put_c('<');
    p_buf.put_c('/');
    if (exer) write_ns_prefix(p_td, p_buf);
    p_buf.put_s((size_t)p_td.namelens[exer] - 1 + indenting, (const unsigned char*)p_td.names[exer]);
    return (int)p_buf.get_len() - encoded_length;
  }

  int EXTERNALtransfer::XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& reader,
                                   unsigned int flavor, unsigned int flavor2, embed_values_dec_struct_t*)
  {
    boolean exer  = is_exer(flavor);
    int success = reader.Ok(), depth = -1;
    for (; success == 1; success = reader.Read()) {
      int type = reader.NodeType();
      if (XML_READER_TYPE_ELEMENT == type) {
        // If our parent is optional and there is an unexpected tag then return and
        // we stay unbound.
        if ((flavor & XER_OPTIONAL) && !check_name((const char*)reader.LocalName(), p_td, exer)) {
          return -1;
        }
        verify_name(reader, p_td, exer);
        depth = reader.Depth();
	reader.Read();
        break;
      }
    }

    field_direct__reference      .XER_decode(EXTERNAL_direct_reference_xer_     , reader, flavor, flavor2, 0);
    field_indirect__reference    .XER_decode(EXTERNAL_indirect_reference_xer_   , reader, flavor, flavor2, 0);
    field_data__value__descriptor.XER_decode(EXTERNAL_data_value_descriptor_xer_, reader, flavor, flavor2, 0);
    field_encoding               .XER_decode(EXTERNAL_encoding_xer_             , reader, flavor, flavor2, 0);

    for (success = reader.Read(); success == 1; success = reader.Read()) {
      int type = reader.NodeType();
      if (XML_READER_TYPE_END_ELEMENT == type) {
        verify_end(reader, p_td, depth, exer);
	reader.Read(); // one more time
        break;
      }
    }
    return 1; // decode successful
  }
} // end of anonymous namespace

/*
 * And this is the EXTERNAL type stuff.
 */

/**
 * This is written by hand, do not delete it! :)
 */
ASN_BER_TLV_t* EXTERNAL::BER_encode_TLV(const TTCN_Typedescriptor_t& p_td, unsigned p_coding) const
{
  EXTERNALtransfer v_tmpmfr;
  v_tmpmfr.load(*this);
  return v_tmpmfr.BER_encode_TLV(p_td, p_coding);
}

/** Load information from an EXTERNALtransfer
 *
 * @param x pointer to an EXTERNALtransfer. It is of type void* because
 * <anonymous-namespace>::EXTERNALtransfer can not appear in the header.
 *
 * Called by XER_decode() */
void EXTERNAL::transfer(void *x)
{
  EXTERNALtransfer & v_tmpmfr = *(EXTERNALtransfer*)x;
  if (v_tmpmfr.direct__reference().ispresent()) {
    if (v_tmpmfr.indirect__reference().ispresent()) {
      EXTERNAL_identification_context__negotiation& v_tmpjsz =
        field_identification.context__negotiation();
      v_tmpjsz.presentation__context__id() = v_tmpmfr.indirect__reference()();
      v_tmpjsz.transfer__syntax() = v_tmpmfr.direct__reference()();
    } else {
      field_identification.syntax() = v_tmpmfr.direct__reference()();
    }
  } else {
    if (v_tmpmfr.indirect__reference().ispresent()) {
      field_identification.presentation__context__id() =
        v_tmpmfr.indirect__reference()();
    } else {
      TTCN_EncDec_ErrorContext::warning("Neither direct-reference nor "
                                        "indirect-reference is present.");
    }
  }
  switch (field_identification.get_selection()) {
  case EXTERNAL_identification::ALT_syntaxes:
  case EXTERNAL_identification::ALT_transfer__syntax:
  case EXTERNAL_identification::ALT_fixed:
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG,
      "EXTERNAL type does not allow the syntaxes, transfer-syntax or fixed");
    break;

  default:
    break; // rest are OK
  }

  field_data__value__descriptor = v_tmpmfr.data__value__descriptor();
  const EXTERNALtransfer_encoding& v_tmpjsz = v_tmpmfr.encoding();
  switch (v_tmpjsz.get_selection()) {
  case EXTERNALtransfer_encoding::ALT_single__ASN1__type:
    field_data__value = v_tmpjsz.single__ASN1__type();
    break;
  case EXTERNALtransfer_encoding::ALT_octet__aligned:
    field_data__value = v_tmpjsz.octet__aligned();
    break;
  case EXTERNALtransfer_encoding::ALT_arbitrary:
    field_data__value = bit2oct(v_tmpjsz.arbitrary());
    break;
  default:
    TTCN_EncDec_ErrorContext::error_internal("Unknown selection for field "
                                             "`encoding' in EXTERNAL type.");
  } // switch
}

/**
 * This is written by hand, do not delete it! :)
 */
boolean EXTERNAL::BER_decode_TLV(const TTCN_Typedescriptor_t& p_td, const ASN_BER_TLV_t& p_tlv, unsigned L_form)
{
  EXTERNALtransfer v_tmpmfr;
  if(!v_tmpmfr.BER_decode_TLV(p_td, p_tlv, L_form))
    return FALSE;
  transfer(&v_tmpmfr);
  return TRUE;
}

int EXTERNAL::XER_encode(const XERdescriptor_t& p_td,
  TTCN_Buffer& p_buf, unsigned int flavor, unsigned int flavor2, int indent, embed_values_enc_struct_t*) const
{
  if(!is_bound()) {
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_UNBOUND, "Encoding an unbound value.");
  }
  EXTERNALtransfer xfer;
  xfer.load(*this);
  return xfer.XER_encode(p_td, p_buf, flavor, flavor2, indent, 0);
}

int EXTERNAL::XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& reader,
  unsigned int flavor, unsigned int flavor2, embed_values_dec_struct_t*)
{
  EXTERNALtransfer xfer;
  xfer.XER_decode(p_td, reader, flavor, flavor2, 0);
  transfer(&xfer);
  return 1; // decode successful
}

/* generated stuff */

void EXTERNAL_identification::clean_up()
{
  switch (union_selection) {
  case ALT_syntaxes:
    delete field_syntaxes;
    break;
  case ALT_syntax:
    delete field_syntax;
    break;
  case ALT_presentation__context__id:
    delete field_presentation__context__id;
    break;
  case ALT_context__negotiation:
    delete field_context__negotiation;
    break;
  case ALT_transfer__syntax:
    delete field_transfer__syntax;
    break;
  case ALT_fixed:
    delete field_fixed;
    break;
  default:
    break;
  }
  union_selection = UNBOUND_VALUE;
}

void EXTERNAL_identification::copy_value(const EXTERNAL_identification& other_value)
{
  switch (other_value.union_selection) {
  case ALT_syntaxes:
    field_syntaxes = new EXTERNAL_identification_syntaxes(*other_value.field_syntaxes);
    break;
  case ALT_syntax:
    field_syntax = new OBJID(*other_value.field_syntax);
    break;
  case ALT_presentation__context__id:
    field_presentation__context__id = new INTEGER(*other_value.field_presentation__context__id);
    break;
  case ALT_context__negotiation:
    field_context__negotiation = new EXTERNAL_identification_context__negotiation(*other_value.field_context__negotiation);
    break;
  case ALT_transfer__syntax:
    field_transfer__syntax = new OBJID(*other_value.field_transfer__syntax);
    break;
  case ALT_fixed:
    field_fixed = new ASN_NULL(*other_value.field_fixed);
    break;
  default:
    TTCN_error("Assignment of an unbound union value of type EXTERNAL.identification.");
  }
  union_selection = other_value.union_selection;
}

EXTERNAL_identification::EXTERNAL_identification()
{
  union_selection = UNBOUND_VALUE;
}

EXTERNAL_identification::EXTERNAL_identification(const EXTERNAL_identification& other_value)
: Base_Type(other_value)
{
  copy_value(other_value);
}

EXTERNAL_identification::~EXTERNAL_identification()
{
  clean_up();
}

EXTERNAL_identification& EXTERNAL_identification::operator=(const EXTERNAL_identification& other_value)
{
  if (this != &other_value) {
    clean_up();
    copy_value(other_value);
  }
  return *this;
}

boolean EXTERNAL_identification::operator==(const EXTERNAL_identification& other_value) const
{
  if (union_selection == UNBOUND_VALUE) TTCN_error("The left operand of comparison is an unbound value of union type EXTERNAL.identification.");
  if (other_value.union_selection == UNBOUND_VALUE) TTCN_error("The right operand of comparison is an unbound value of union type EXTERNAL.identification.");
  if (union_selection != other_value.union_selection) return FALSE;
  switch (union_selection) {
  case ALT_syntaxes:
    return *field_syntaxes == *other_value.field_syntaxes;
  case ALT_syntax:
    return *field_syntax == *other_value.field_syntax;
  case ALT_presentation__context__id:
    return *field_presentation__context__id == *other_value.field_presentation__context__id;
  case ALT_context__negotiation:
    return *field_context__negotiation == *other_value.field_context__negotiation;
  case ALT_transfer__syntax:
    return *field_transfer__syntax == *other_value.field_transfer__syntax;
  case ALT_fixed:
    return *field_fixed == *other_value.field_fixed;
  default:
    return FALSE;
  }
}

EXTERNAL_identification_syntaxes& EXTERNAL_identification::syntaxes()
{
  if (union_selection != ALT_syntaxes) {
    clean_up();
    field_syntaxes = new EXTERNAL_identification_syntaxes;
    union_selection = ALT_syntaxes;
  }
  return *field_syntaxes;
}

const EXTERNAL_identification_syntaxes& EXTERNAL_identification::syntaxes() const
{
  if (union_selection != ALT_syntaxes) TTCN_error("Using non-selected field syntaxes in a value of union type EXTERNAL.identification.");
  return *field_syntaxes;
}

OBJID& EXTERNAL_identification::syntax()
{
  if (union_selection != ALT_syntax) {
    clean_up();
    field_syntax = new OBJID;
    union_selection = ALT_syntax;
  }
  return *field_syntax;
}

const OBJID& EXTERNAL_identification::syntax() const
{
  if (union_selection != ALT_syntax) TTCN_error("Using non-selected field syntax in a value of union type EXTERNAL.identification.");
  return *field_syntax;
}

INTEGER& EXTERNAL_identification::presentation__context__id()
{
  if (union_selection != ALT_presentation__context__id) {
    clean_up();
    field_presentation__context__id = new INTEGER;
    union_selection = ALT_presentation__context__id;
  }
  return *field_presentation__context__id;
}

const INTEGER& EXTERNAL_identification::presentation__context__id() const
{
  if (union_selection != ALT_presentation__context__id) TTCN_error("Using non-selected field presentation_context_id in a value of union type EXTERNAL.identification.");
  return *field_presentation__context__id;
}

EXTERNAL_identification_context__negotiation& EXTERNAL_identification::context__negotiation()
{
  if (union_selection != ALT_context__negotiation) {
    clean_up();
    field_context__negotiation = new EXTERNAL_identification_context__negotiation;
    union_selection = ALT_context__negotiation;
  }
  return *field_context__negotiation;
}

const EXTERNAL_identification_context__negotiation& EXTERNAL_identification::context__negotiation() const
{
  if (union_selection != ALT_context__negotiation) TTCN_error("Using non-selected field context_negotiation in a value of union type EXTERNAL.identification.");
  return *field_context__negotiation;
}

OBJID& EXTERNAL_identification::transfer__syntax()
{
  if (union_selection != ALT_transfer__syntax) {
    clean_up();
    field_transfer__syntax = new OBJID;
    union_selection = ALT_transfer__syntax;
  }
  return *field_transfer__syntax;
}

const OBJID& EXTERNAL_identification::transfer__syntax() const
{
  if (union_selection != ALT_transfer__syntax) TTCN_error("Using non-selected field transfer_syntax in a value of union type EXTERNAL.identification.");
  return *field_transfer__syntax;
}

ASN_NULL& EXTERNAL_identification::fixed()
{
  if (union_selection != ALT_fixed) {
    clean_up();
    field_fixed = new ASN_NULL;
    union_selection = ALT_fixed;
  }
  return *field_fixed;
}

const ASN_NULL& EXTERNAL_identification::fixed() const
{
  if (union_selection != ALT_fixed) TTCN_error("Using non-selected field fixed in a value of union type EXTERNAL.identification.");
  return *field_fixed;
}

boolean EXTERNAL_identification::ischosen(union_selection_type checked_selection) const
{
  if (checked_selection == UNBOUND_VALUE) TTCN_error("Internal error: Performing ischosen() operation on an invalid field of union type EXTERNAL.identification.");
  if (union_selection == UNBOUND_VALUE) TTCN_error("Internal error: Performing ischosen() operation on an unbound value of union type EXTERNAL.identification.");
  return union_selection == checked_selection;
}

boolean EXTERNAL_identification::is_value() const
{
  switch (union_selection) {
  case ALT_syntaxes:
    return field_syntaxes->is_value();
  case ALT_syntax:
    return field_syntax->is_value();
  case ALT_presentation__context__id:
    return field_presentation__context__id->is_value();
  case ALT_context__negotiation:
    return field_context__negotiation->is_value();
  case ALT_transfer__syntax:
    return field_transfer__syntax->is_value();
  case ALT_fixed:
    return field_fixed->is_value();
  default:
    return FALSE;
  }
}

void EXTERNAL_identification::log() const
{
  switch (union_selection) {
  case ALT_syntaxes:
    TTCN_Logger::log_event_str("{ syntaxes := ");
    field_syntaxes->log();
    TTCN_Logger::log_event_str(" }");
    break;
  case ALT_syntax:
    TTCN_Logger::log_event_str("{ syntax := ");
    field_syntax->log();
    TTCN_Logger::log_event_str(" }");
    break;
  case ALT_presentation__context__id:
    TTCN_Logger::log_event_str("{ presentation_context_id := ");
    field_presentation__context__id->log();
    TTCN_Logger::log_event_str(" }");
    break;
  case ALT_context__negotiation:
    TTCN_Logger::log_event_str("{ context_negotiation := ");
    field_context__negotiation->log();
    TTCN_Logger::log_event_str(" }");
    break;
  case ALT_transfer__syntax:
    TTCN_Logger::log_event_str("{ transfer_syntax := ");
    field_transfer__syntax->log();
    TTCN_Logger::log_event_str(" }");
    break;
  case ALT_fixed:
    TTCN_Logger::log_event_str("{ fixed := ");
    field_fixed->log();
    TTCN_Logger::log_event_str(" }");
    break;
  default:
    TTCN_Logger::log_event_str("<unbound>");
    break;
  }
}

void EXTERNAL_identification::set_param(Module_Param& param) {
  param.basic_check(Module_Param::BC_VALUE, "union value");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  if (mp->get_type()==Module_Param::MP_Value_List && mp->get_size()==0) return;
  if (mp->get_type()!=Module_Param::MP_Assignment_List) {
    param.error("union value with field name was expected");
  }
  Module_Param* mp_last = mp->get_elem(mp->get_size()-1);
  if (!strcmp(mp_last->get_id()->get_name(), "syntaxes")) {
    syntaxes().set_param(*mp_last);
    return;
  }
  if (!strcmp(mp_last->get_id()->get_name(), "syntax")) {
    syntax().set_param(*mp_last);
    return;
  }
  if (!strcmp(mp_last->get_id()->get_name(), "presentation_context_id")) {
    presentation__context__id().set_param(*mp_last);
    return;
  }
  if (!strcmp(mp_last->get_id()->get_name(), "context_negotiation")) {
    context__negotiation().set_param(*mp_last);
    return;
  }
  if (!strcmp(mp_last->get_id()->get_name(), "transfer_syntax")) {
    transfer__syntax().set_param(*mp_last);
    return;
  }
  if (!strcmp(mp_last->get_id()->get_name(), "fixed")) {
    fixed().set_param(*mp_last);
    return;
  }
  mp_last->error("Field %s does not exist in type EXTERNAL.identification.", mp_last->get_id()->get_name());
}

#ifdef TITAN_RUNTIME_2
Module_Param* EXTERNAL_identification::get_param(Module_Param_Name& param_name) const
{
  if (!is_bound()) {
    return new Module_Param_Unbound();
  }
  Module_Param* mp_field = NULL;
  
  switch(get_selection()) {
  case ALT_syntaxes:
    mp_field = field_syntaxes->get_param(param_name);
    mp_field->set_id(new Module_Param_FieldName(mcopystr("syntaxes")));
    break;
  case ALT_syntax:
    mp_field = field_syntax->get_param(param_name);
    mp_field->set_id(new Module_Param_FieldName(mcopystr("syntax")));
    break;
  case ALT_presentation__context__id:
    mp_field = field_presentation__context__id->get_param(param_name);
    mp_field->set_id(new Module_Param_FieldName(mcopystr("presentation_context_id")));
    break;
  case ALT_context__negotiation:
    mp_field = field_context__negotiation->get_param(param_name);
    mp_field->set_id(new Module_Param_FieldName(mcopystr("context_negotiation")));
    break;
  case ALT_transfer__syntax:
    mp_field = field_transfer__syntax->get_param(param_name);
    mp_field->set_id(new Module_Param_FieldName(mcopystr("transfer_syntax")));
    break;
  case ALT_fixed:
    mp_field = field_fixed->get_param(param_name);
    mp_field->set_id(new Module_Param_FieldName(mcopystr("fixed")));
    break;
  default:
    break;
  }
  Module_Param_Assignment_List* mp = new Module_Param_Assignment_List();
  mp->add_elem(mp_field);
  return mp;
}
#endif

void EXTERNAL_identification_template::set_param(Module_Param& param)
{
  param.basic_check(Module_Param::BC_TEMPLATE, "union template");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  switch (mp->get_type()) {
  case Module_Param::MP_Omit:
    *this = OMIT_VALUE;
    break;
  case Module_Param::MP_Any:
    *this = ANY_VALUE;
    break;
  case Module_Param::MP_AnyOrNone:
    *this = ANY_OR_OMIT;
    break;
  case Module_Param::MP_List_Template:
  case Module_Param::MP_ComplementList_Template: {
    EXTERNAL_identification_template temp;
    temp.set_type(mp->get_type() == Module_Param::MP_List_Template ?
      VALUE_LIST : COMPLEMENTED_LIST, mp->get_size());
    for (size_t i=0; i<mp->get_size(); i++) {
      temp.list_item(i).set_param(*mp->get_elem(i));
    }
    *this = temp;
    break; }
  case Module_Param::MP_Value_List:
    if (mp->get_size()==0) break;
    param.type_error("union template", "EXTERNAL.identification");
    break;
  case Module_Param::MP_Assignment_List: {
    Module_Param* mp_last = mp->get_elem(mp->get_size()-1);
    if (!strcmp(mp_last->get_id()->get_name(), "syntaxes")) {
      syntaxes().set_param(*mp_last);
      break;
    }
    if (!strcmp(mp_last->get_id()->get_name(), "syntax")) {
      syntax().set_param(*mp_last);
      break;
    }
    if (!strcmp(mp_last->get_id()->get_name(), "presentation_context_id")) {
      presentation__context__id().set_param(*mp_last);
      break;
    }
    if (!strcmp(mp_last->get_id()->get_name(), "context_negotiation")) {
      context__negotiation().set_param(*mp_last);
      break;
    }
    if (!strcmp(mp_last->get_id()->get_name(), "transfer_syntax")) {
      transfer__syntax().set_param(*mp_last);
      break;
    }
    if (!strcmp(mp_last->get_id()->get_name(), "fixed")) {
      fixed().set_param(*mp_last);
      break;
    }
    mp_last->error("Field %s does not exist in type EXTERNAL.identification.", mp_last->get_id()->get_name());
  } break;
  default:
    param.type_error("union template", "EXTERNAL.identification");
  }
  is_ifpresent = param.get_ifpresent() || mp->get_ifpresent();
}

#ifdef TITAN_RUNTIME_2
Module_Param* EXTERNAL_identification_template::get_param(Module_Param_Name& param_name) const
{
  Module_Param* mp = NULL;
  switch (template_selection) {
  case UNINITIALIZED_TEMPLATE:
    mp = new Module_Param_Unbound();
    break;
  case OMIT_VALUE:
    mp = new Module_Param_Omit();
    break;
  case ANY_VALUE:
    mp = new Module_Param_Any();
    break;
  case ANY_OR_OMIT:
    mp = new Module_Param_AnyOrNone();
    break;
  case SPECIFIC_VALUE: {
    Module_Param* mp_field = NULL;
    switch(single_value.union_selection) {
    case EXTERNAL_identification::ALT_syntaxes:
      mp_field = single_value.field_syntaxes->get_param(param_name);
      mp_field->set_id(new Module_Param_FieldName(mcopystr("syntaxes")));
      break;
    case EXTERNAL_identification::ALT_syntax:
      mp_field = single_value.field_syntax->get_param(param_name);
      mp_field->set_id(new Module_Param_FieldName(mcopystr("syntax")));
      break;
    case EXTERNAL_identification::ALT_presentation__context__id:
      mp_field = single_value.field_presentation__context__id->get_param(param_name);
      mp_field->set_id(new Module_Param_FieldName(mcopystr("presentation_context_id")));
      break;
    case EXTERNAL_identification::ALT_context__negotiation:
      mp_field = single_value.field_context__negotiation->get_param(param_name);
      mp_field->set_id(new Module_Param_FieldName(mcopystr("context_negotiation")));
      break;
    case EXTERNAL_identification::ALT_transfer__syntax:
      mp_field = single_value.field_transfer__syntax->get_param(param_name);
      mp_field->set_id(new Module_Param_FieldName(mcopystr("transfer_syntax")));
      break;
    case EXTERNAL_identification::ALT_fixed:
      mp_field = single_value.field_fixed->get_param(param_name);
      mp_field->set_id(new Module_Param_FieldName(mcopystr("fixed")));
      break;
    default:
      break;
    }
    mp = new Module_Param_Assignment_List();
    mp->add_elem(mp_field);
    break; }
  case VALUE_LIST:
  case COMPLEMENTED_LIST: {
    if (template_selection == VALUE_LIST) {
      mp = new Module_Param_List_Template();
    }
    else {
      mp = new Module_Param_ComplementList_Template();
    }
    for (size_t i = 0; i < value_list.n_values; ++i) {
      mp->add_elem(value_list.list_value[i].get_param(param_name));
    }
    break; }
  default:
    TTCN_error("Referencing an uninitialized/unsupported value of type EXTERNAL.identification.");
    break;
  }
  if (is_ifpresent) {
    mp->set_ifpresent();
  }
  return mp;
}
#endif

void EXTERNAL_identification::encode_text(Text_Buf& text_buf) const
{
  text_buf.push_int(union_selection);
  switch (union_selection) {
  case ALT_syntaxes:
    field_syntaxes->encode_text(text_buf);
    break;
  case ALT_syntax:
    field_syntax->encode_text(text_buf);
    break;
  case ALT_presentation__context__id:
    field_presentation__context__id->encode_text(text_buf);
    break;
  case ALT_context__negotiation:
    field_context__negotiation->encode_text(text_buf);
    break;
  case ALT_transfer__syntax:
    field_transfer__syntax->encode_text(text_buf);
    break;
  case ALT_fixed:
    field_fixed->encode_text(text_buf);
    break;
  default:
    TTCN_error("Text encoder: Encoding an unbound value of union type EXTERNAL.identification.");
  }
}

void EXTERNAL_identification::decode_text(Text_Buf& text_buf)
{
  switch ((union_selection_type)text_buf.pull_int().get_val()) {
  case ALT_syntaxes:
    syntaxes().decode_text(text_buf);
    break;
  case ALT_syntax:
    syntax().decode_text(text_buf);
    break;
  case ALT_presentation__context__id:
    presentation__context__id().decode_text(text_buf);
    break;
  case ALT_context__negotiation:
    context__negotiation().decode_text(text_buf);
    break;
  case ALT_transfer__syntax:
    transfer__syntax().decode_text(text_buf);
    break;
  case ALT_fixed:
    fixed().decode_text(text_buf);
    break;
  default:
    TTCN_error("Text decoder: Unrecognized union selector was received for type EXTERNAL.identification.");
  }
}


void EXTERNAL_identification_template::clean_up()
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    switch (single_value.union_selection) {
    case EXTERNAL_identification::ALT_syntaxes:
      delete single_value.field_syntaxes;
      break;
    case EXTERNAL_identification::ALT_syntax:
      delete single_value.field_syntax;
      break;
    case EXTERNAL_identification::ALT_presentation__context__id:
      delete single_value.field_presentation__context__id;
      break;
    case EXTERNAL_identification::ALT_context__negotiation:
      delete single_value.field_context__negotiation;
      break;
    case EXTERNAL_identification::ALT_transfer__syntax:
      delete single_value.field_transfer__syntax;
      break;
    case EXTERNAL_identification::ALT_fixed:
      delete single_value.field_fixed;
      break;
    default:
      break;
    }
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    delete [] value_list.list_value;
    break;
  default:
    break;
  }
  template_selection = UNINITIALIZED_TEMPLATE;
}

void EXTERNAL_identification_template::copy_value(const EXTERNAL_identification& other_value)
{
  single_value.union_selection = other_value.get_selection();
  switch (single_value.union_selection) {
  case EXTERNAL_identification::ALT_syntaxes:
    single_value.field_syntaxes = new EXTERNAL_identification_syntaxes_template(other_value.syntaxes());
    break;
  case EXTERNAL_identification::ALT_syntax:
    single_value.field_syntax = new OBJID_template(other_value.syntax());
    break;
  case EXTERNAL_identification::ALT_presentation__context__id:
    single_value.field_presentation__context__id = new INTEGER_template(other_value.presentation__context__id());
    break;
  case EXTERNAL_identification::ALT_context__negotiation:
    single_value.field_context__negotiation = new EXTERNAL_identification_context__negotiation_template(other_value.context__negotiation());
    break;
  case EXTERNAL_identification::ALT_transfer__syntax:
    single_value.field_transfer__syntax = new OBJID_template(other_value.transfer__syntax());
    break;
  case EXTERNAL_identification::ALT_fixed:
    single_value.field_fixed = new ASN_NULL_template(other_value.fixed());
    break;
  default:
    TTCN_error("Initializing a template with an unbound value of type EXTERNAL.identification.");
  }
  set_selection(SPECIFIC_VALUE);
}

void EXTERNAL_identification_template::copy_template(const EXTERNAL_identification_template& other_value)
{
  switch (other_value.template_selection) {
  case SPECIFIC_VALUE:
    single_value.union_selection = other_value.single_value.union_selection;
    switch (single_value.union_selection) {
    case EXTERNAL_identification::ALT_syntaxes:
      single_value.field_syntaxes = new EXTERNAL_identification_syntaxes_template(*other_value.single_value.field_syntaxes);
      break;
    case EXTERNAL_identification::ALT_syntax:
      single_value.field_syntax = new OBJID_template(*other_value.single_value.field_syntax);
      break;
    case EXTERNAL_identification::ALT_presentation__context__id:
      single_value.field_presentation__context__id = new INTEGER_template(*other_value.single_value.field_presentation__context__id);
      break;
    case EXTERNAL_identification::ALT_context__negotiation:
      single_value.field_context__negotiation = new EXTERNAL_identification_context__negotiation_template(*other_value.single_value.field_context__negotiation);
      break;
    case EXTERNAL_identification::ALT_transfer__syntax:
      single_value.field_transfer__syntax = new OBJID_template(*other_value.single_value.field_transfer__syntax);
      break;
    case EXTERNAL_identification::ALT_fixed:
      single_value.field_fixed = new ASN_NULL_template(*other_value.single_value.field_fixed);
      break;
    default:
      TTCN_error("Internal error: Invalid union selector in a specific value when copying a template of type EXTERNAL.identification.");
    }
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = other_value.value_list.n_values;
    value_list.list_value = new EXTERNAL_identification_template[value_list.n_values];
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count].copy_template(other_value.value_list.list_value[list_count]);
    break;
  default:
    TTCN_error("Copying an uninitialized template of union type EXTERNAL.identification.");
  }
  set_selection(other_value);
}

EXTERNAL_identification_template::EXTERNAL_identification_template()
{
}

EXTERNAL_identification_template::EXTERNAL_identification_template(template_sel other_value)
  : Base_Template(other_value)
{
  check_single_selection(other_value);
}

EXTERNAL_identification_template::EXTERNAL_identification_template(const EXTERNAL_identification& other_value)
{
  copy_value(other_value);
}

EXTERNAL_identification_template::EXTERNAL_identification_template(const OPTIONAL<EXTERNAL_identification>& other_value)
{
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    copy_value((const EXTERNAL_identification&)other_value);
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Creating a template of union type EXTERNAL.identification from an unbound optional field.");
  }
}

EXTERNAL_identification_template::EXTERNAL_identification_template(const EXTERNAL_identification_template& other_value)
: Base_Template()
{
  copy_template(other_value);
}

EXTERNAL_identification_template::~EXTERNAL_identification_template()
{
  clean_up();
}

EXTERNAL_identification_template& EXTERNAL_identification_template::operator=(template_sel other_value)
{
  check_single_selection(other_value);
  clean_up();
  set_selection(other_value);
  return *this;
}

EXTERNAL_identification_template& EXTERNAL_identification_template::operator=(const EXTERNAL_identification& other_value)
{
  clean_up();
  copy_value(other_value);
  return *this;
}

EXTERNAL_identification_template& EXTERNAL_identification_template::operator=(const OPTIONAL<EXTERNAL_identification>& other_value)
{
  clean_up();
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    copy_value((const EXTERNAL_identification&)other_value);
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Assignment of an unbound optional field to a template of union type EXTERNAL.identification.");
  }
  return *this;
}

EXTERNAL_identification_template& EXTERNAL_identification_template::operator=(const EXTERNAL_identification_template& other_value)
{
  if (&other_value != this) {
    clean_up();
    copy_template(other_value);
  }
  return *this;
}

boolean EXTERNAL_identification_template::match(const EXTERNAL_identification& other_value,
                                                boolean /* legacy */) const
{
  switch (template_selection) {
  case ANY_VALUE:
  case ANY_OR_OMIT:
    return TRUE;
  case OMIT_VALUE:
    return FALSE;
  case SPECIFIC_VALUE:
    {
      EXTERNAL_identification::union_selection_type value_selection = other_value.get_selection();
      if (value_selection == EXTERNAL_identification::UNBOUND_VALUE) return FALSE;
      if (value_selection != single_value.union_selection) return FALSE;
      switch (value_selection) {
      case EXTERNAL_identification::ALT_syntaxes:
        return single_value.field_syntaxes->match(other_value.syntaxes());
      case EXTERNAL_identification::ALT_syntax:
        return single_value.field_syntax->match(other_value.syntax());
      case EXTERNAL_identification::ALT_presentation__context__id:
        return single_value.field_presentation__context__id->match(other_value.presentation__context__id());
      case EXTERNAL_identification::ALT_context__negotiation:
        return single_value.field_context__negotiation->match(other_value.context__negotiation());
      case EXTERNAL_identification::ALT_transfer__syntax:
        return single_value.field_transfer__syntax->match(other_value.transfer__syntax());
      case EXTERNAL_identification::ALT_fixed:
        return single_value.field_fixed->match(other_value.fixed());
      default:
        TTCN_error("Internal error: Invalid selector in a specific value when matching a template of union type EXTERNAL.identification.");
      }
    }
    break; // should never get here
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      if (value_list.list_value[list_count].match(other_value)) return template_selection == VALUE_LIST;
    return template_selection == COMPLEMENTED_LIST;
  default:
    TTCN_error ("Matching an uninitialized template of union type EXTERNAL.identification.");
  }
  return FALSE;
}

EXTERNAL_identification EXTERNAL_identification_template::valueof() const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Performing a valueof or send operation on a non-specific template of union type EXTERNAL.identification.");
  EXTERNAL_identification ret_val;
  switch (single_value.union_selection) {
  case EXTERNAL_identification::ALT_syntaxes:
    ret_val.syntaxes() = single_value.field_syntaxes->valueof();
    break;
  case EXTERNAL_identification::ALT_syntax:
    ret_val.syntax() = single_value.field_syntax->valueof();
    break;
  case EXTERNAL_identification::ALT_presentation__context__id:
    ret_val.presentation__context__id() = single_value.field_presentation__context__id->valueof();
    break;
  case EXTERNAL_identification::ALT_context__negotiation:
    ret_val.context__negotiation() = single_value.field_context__negotiation->valueof();
    break;
  case EXTERNAL_identification::ALT_transfer__syntax:
    ret_val.transfer__syntax() = single_value.field_transfer__syntax->valueof();
    break;
  case EXTERNAL_identification::ALT_fixed:
    ret_val.fixed() = single_value.field_fixed->valueof();
    break;
  default:
    TTCN_error("Internal error: Invalid selector in a specific value when performing valueof operation on a template of union type EXTERNAL.identification.");
  }
  return ret_val;
}

EXTERNAL_identification_template& EXTERNAL_identification_template::list_item(unsigned int list_index) const
{
  if (template_selection != VALUE_LIST && template_selection != COMPLEMENTED_LIST) TTCN_error("Internal error: Accessing a list element of a non-list template of union type EXTERNAL.identification.");
  if (list_index >= value_list.n_values) TTCN_error("Internal error: Index overflow in a value list template of union type EXTERNAL.identification.");
  return value_list.list_value[list_index];
}
void EXTERNAL_identification_template::set_type(template_sel template_type, unsigned int list_length)
{
  if (template_type != VALUE_LIST && template_type != COMPLEMENTED_LIST) TTCN_error ("Internal error: Setting an invalid list for a template of union type EXTERNAL.identification.");
  clean_up();
  set_selection(template_type);
  value_list.n_values = list_length;
  value_list.list_value = new EXTERNAL_identification_template[list_length];
}

EXTERNAL_identification_syntaxes_template& EXTERNAL_identification_template::syntaxes()
{
  if (template_selection != SPECIFIC_VALUE || single_value.union_selection != EXTERNAL_identification::ALT_syntaxes) {
    template_sel old_selection = template_selection;
    clean_up();
    if (old_selection == ANY_VALUE || old_selection == ANY_OR_OMIT) single_value.field_syntaxes = new EXTERNAL_identification_syntaxes_template(ANY_VALUE);
    else single_value.field_syntaxes = new EXTERNAL_identification_syntaxes_template;
    single_value.union_selection = EXTERNAL_identification::ALT_syntaxes;
    set_selection(SPECIFIC_VALUE);
  }
  return *single_value.field_syntaxes;
}

const EXTERNAL_identification_syntaxes_template& EXTERNAL_identification_template::syntaxes() const
{
  if (template_selection != SPECIFIC_VALUE) TTCN_error("Accessing field syntaxes in a non-specific template of union type EXTERNAL.identification.");
  if (single_value.union_selection != EXTERNAL_identification::ALT_syntaxes) TTCN_error("Accessing non-selected field syntaxes in a template of union type EXTERNAL.identification.");
  return *single_value.field_syntaxes;
}

OBJID_template& EXTERNAL_identification_template::syntax()
{
  if (template_selection != SPECIFIC_VALUE || single_value.union_selection != EXTERNAL_identification::ALT_syntax) {
    template_sel old_selection = template_selection;
    clean_up();
    if (old_selection == ANY_VALUE || old_selection == ANY_OR_OMIT) single_value.field_syntax = new OBJID_template(ANY_VALUE);
    else single_value.field_syntax = new OBJID_template;
    single_value.union_selection = EXTERNAL_identification::ALT_syntax;
    set_selection(SPECIFIC_VALUE);
  }
  return *single_value.field_syntax;
}

const OBJID_template& EXTERNAL_identification_template::syntax() const
{
  if (template_selection != SPECIFIC_VALUE) TTCN_error("Accessing field syntax in a non-specific template of union type EXTERNAL.identification.");
  if (single_value.union_selection != EXTERNAL_identification::ALT_syntax) TTCN_error("Accessing non-selected field syntax in a template of union type EXTERNAL.identification.");
  return *single_value.field_syntax;
}

INTEGER_template& EXTERNAL_identification_template::presentation__context__id()
{
  if (template_selection != SPECIFIC_VALUE || single_value.union_selection != EXTERNAL_identification::ALT_presentation__context__id) {
    template_sel old_selection = template_selection;
    clean_up();
    if (old_selection == ANY_VALUE || old_selection == ANY_OR_OMIT) single_value.field_presentation__context__id = new INTEGER_template(ANY_VALUE);
    else single_value.field_presentation__context__id = new INTEGER_template;
    single_value.union_selection = EXTERNAL_identification::ALT_presentation__context__id;
    set_selection(SPECIFIC_VALUE);
  }
  return *single_value.field_presentation__context__id;
}

const INTEGER_template& EXTERNAL_identification_template::presentation__context__id() const
{
  if (template_selection != SPECIFIC_VALUE) TTCN_error("Accessing field presentation_context_id in a non-specific template of union type EXTERNAL.identification.");
  if (single_value.union_selection != EXTERNAL_identification::ALT_presentation__context__id) TTCN_error("Accessing non-selected field presentation_context_id in a template of union type EXTERNAL.identification.");
  return *single_value.field_presentation__context__id;
}

EXTERNAL_identification_context__negotiation_template& EXTERNAL_identification_template::context__negotiation()
{
  if (template_selection != SPECIFIC_VALUE || single_value.union_selection != EXTERNAL_identification::ALT_context__negotiation) {
    template_sel old_selection = template_selection;
    clean_up();
    if (old_selection == ANY_VALUE || old_selection == ANY_OR_OMIT) single_value.field_context__negotiation = new EXTERNAL_identification_context__negotiation_template(ANY_VALUE);
    else single_value.field_context__negotiation = new EXTERNAL_identification_context__negotiation_template;
    single_value.union_selection = EXTERNAL_identification::ALT_context__negotiation;
    set_selection(SPECIFIC_VALUE);
  }
  return *single_value.field_context__negotiation;
}

const EXTERNAL_identification_context__negotiation_template& EXTERNAL_identification_template::context__negotiation() const
{
  if (template_selection != SPECIFIC_VALUE) TTCN_error("Accessing field context_negotiation in a non-specific template of union type EXTERNAL.identification.");
  if (single_value.union_selection != EXTERNAL_identification::ALT_context__negotiation) TTCN_error("Accessing non-selected field context_negotiation in a template of union type EXTERNAL.identification.");
  return *single_value.field_context__negotiation;
}

OBJID_template& EXTERNAL_identification_template::transfer__syntax()
{
  if (template_selection != SPECIFIC_VALUE || single_value.union_selection != EXTERNAL_identification::ALT_transfer__syntax) {
    template_sel old_selection = template_selection;
    clean_up();
    if (old_selection == ANY_VALUE || old_selection == ANY_OR_OMIT) single_value.field_transfer__syntax = new OBJID_template(ANY_VALUE);
    else single_value.field_transfer__syntax = new OBJID_template;
    single_value.union_selection = EXTERNAL_identification::ALT_transfer__syntax;
    set_selection(SPECIFIC_VALUE);
  }
  return *single_value.field_transfer__syntax;
}

const OBJID_template& EXTERNAL_identification_template::transfer__syntax() const
{
  if (template_selection != SPECIFIC_VALUE) TTCN_error("Accessing field transfer_syntax in a non-specific template of union type EXTERNAL.identification.");
  if (single_value.union_selection != EXTERNAL_identification::ALT_transfer__syntax) TTCN_error("Accessing non-selected field transfer_syntax in a template of union type EXTERNAL.identification.");
  return *single_value.field_transfer__syntax;
}

ASN_NULL_template& EXTERNAL_identification_template::fixed()
{
  if (template_selection != SPECIFIC_VALUE || single_value.union_selection != EXTERNAL_identification::ALT_fixed) {
    template_sel old_selection = template_selection;
    clean_up();
    if (old_selection == ANY_VALUE || old_selection == ANY_OR_OMIT) single_value.field_fixed = new ASN_NULL_template(ANY_VALUE);
    else single_value.field_fixed = new ASN_NULL_template;
    single_value.union_selection = EXTERNAL_identification::ALT_fixed;
    set_selection(SPECIFIC_VALUE);
  }
  return *single_value.field_fixed;
}

const ASN_NULL_template& EXTERNAL_identification_template::fixed() const
{
  if (template_selection != SPECIFIC_VALUE) TTCN_error("Accessing field fixed in a non-specific template of union type EXTERNAL.identification.");
  if (single_value.union_selection != EXTERNAL_identification::ALT_fixed) TTCN_error("Accessing non-selected field fixed in a template of union type EXTERNAL.identification.");
  return *single_value.field_fixed;
}

boolean EXTERNAL_identification_template::ischosen(EXTERNAL_identification::union_selection_type checked_selection) const
{
  if (checked_selection == EXTERNAL_identification::UNBOUND_VALUE) TTCN_error("Internal error: Performing ischosen() operation on an invalid field of union type EXTERNAL.identification.");
  switch (template_selection) {
  case SPECIFIC_VALUE:
    if (single_value.union_selection == EXTERNAL_identification::UNBOUND_VALUE) TTCN_error("Internal error: Invalid selector in a specific value when performing ischosen() operation on a template of union type EXTERNAL.identification.");
    return single_value.union_selection == checked_selection;
  case VALUE_LIST:
    {
      if (value_list.n_values < 1)
        TTCN_error("Internal error: Performing ischosen() operation on a template of union type EXTERNAL.identification containing an empty list.");
      boolean ret_val = value_list.list_value[0].ischosen(checked_selection);
      boolean all_same = TRUE;
      for (unsigned int list_count = 1; list_count < value_list.n_values; list_count++) {
        if (value_list.list_value[list_count].ischosen(checked_selection) != ret_val) {
          all_same = FALSE;
          break;
        }
      }
      if (all_same) return ret_val;
    }
    // FIXME really no break?
  case ANY_VALUE:
  case ANY_OR_OMIT:
  case OMIT_VALUE:
  case COMPLEMENTED_LIST:
    TTCN_error("Performing ischosen() operation on a template of union type EXTERNAL.identification, which does not determine unambiguously the chosen field of the matching values.");
  default:
    TTCN_error("Performing ischosen() operation on an uninitialized template of union type EXTERNAL.identification");
  }
  return FALSE;
}

void EXTERNAL_identification_template::log() const
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    switch (single_value.union_selection) {
    case EXTERNAL_identification::ALT_syntaxes:
      TTCN_Logger::log_event_str("{ syntaxes := ");
      single_value.field_syntaxes->log();
      TTCN_Logger::log_event_str(" }");
      break;
    case EXTERNAL_identification::ALT_syntax:
      TTCN_Logger::log_event_str("{ syntax := ");
      single_value.field_syntax->log();
      TTCN_Logger::log_event_str(" }");
      break;
    case EXTERNAL_identification::ALT_presentation__context__id:
      TTCN_Logger::log_event_str("{ presentation_context_id := ");
      single_value.field_presentation__context__id->log();
      TTCN_Logger::log_event_str(" }");
      break;
    case EXTERNAL_identification::ALT_context__negotiation:
      TTCN_Logger::log_event_str("{ context_negotiation := ");
      single_value.field_context__negotiation->log();
      TTCN_Logger::log_event_str(" }");
      break;
    case EXTERNAL_identification::ALT_transfer__syntax:
      TTCN_Logger::log_event_str("{ transfer_syntax := ");
      single_value.field_transfer__syntax->log();
      TTCN_Logger::log_event_str(" }");
      break;
    case EXTERNAL_identification::ALT_fixed:
      TTCN_Logger::log_event_str("{ fixed := ");
      single_value.field_fixed->log();
      TTCN_Logger::log_event_str(" }");
      break;
    default:
      TTCN_Logger::log_event_str("<invalid selector>");
      break;
    }
    break;
  case COMPLEMENTED_LIST:
    TTCN_Logger::log_event_str("complement ");
    // no break
  case VALUE_LIST:
    TTCN_Logger::log_char('(');
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++) {
      if (list_count > 0) TTCN_Logger::log_event_str(", ");
      value_list.list_value[list_count].log();
    }
    TTCN_Logger::log_char(')');
    break;
  default:
    log_generic();
    break;
  }
  log_ifpresent();
}

void EXTERNAL_identification_template::log_match(const EXTERNAL_identification& match_value,
                                                 boolean /* legacy */) const
{
  if(TTCN_Logger::VERBOSITY_COMPACT == TTCN_Logger::get_matching_verbosity()){
    if(match(match_value)){
      TTCN_Logger::print_logmatch_buffer();
      TTCN_Logger::log_event_str(" matched ");
    }
    return;
  }
  if (template_selection == SPECIFIC_VALUE && single_value.union_selection == match_value.get_selection()) {
    switch (single_value.union_selection) {
    case EXTERNAL_identification::ALT_syntaxes:
      if(TTCN_Logger::VERBOSITY_COMPACT == TTCN_Logger::get_matching_verbosity()){
        TTCN_Logger::log_logmatch_info(".syntaxes");
        single_value.field_syntaxes->log_match(match_value.syntaxes());
      }else{
      TTCN_Logger::log_event_str("{ syntaxes := ");
      single_value.field_syntaxes->log_match(match_value.syntaxes());
      TTCN_Logger::log_event_str(" }");
      }
      break;
    case EXTERNAL_identification::ALT_syntax:
      if(TTCN_Logger::VERBOSITY_COMPACT == TTCN_Logger::get_matching_verbosity()){
        TTCN_Logger::log_logmatch_info(".syntax");
        single_value.field_syntax->log_match(match_value.syntax());
      }else{
      TTCN_Logger::log_event_str("{ syntax := ");
      single_value.field_syntax->log_match(match_value.syntax());
      TTCN_Logger::log_event_str(" }");
      }
      break;
    case EXTERNAL_identification::ALT_presentation__context__id:
      if(TTCN_Logger::VERBOSITY_COMPACT == TTCN_Logger::get_matching_verbosity()){
        TTCN_Logger::log_logmatch_info(".presentation_context_id");
        single_value.field_presentation__context__id->log_match(match_value.presentation__context__id());
      }else{
      TTCN_Logger::log_event_str("{ presentation_context_id := ");
      single_value.field_presentation__context__id->log_match(match_value.presentation__context__id());
      TTCN_Logger::log_event_str(" }");
      }
      break;
    case EXTERNAL_identification::ALT_context__negotiation:
      if(TTCN_Logger::VERBOSITY_COMPACT == TTCN_Logger::get_matching_verbosity()){
        TTCN_Logger::log_logmatch_info(".context_negotiation");
        single_value.field_context__negotiation->log_match(match_value.context__negotiation());
      }else{
      TTCN_Logger::log_event_str("{ context_negotiation := ");
      single_value.field_context__negotiation->log_match(match_value.context__negotiation());
      TTCN_Logger::log_event_str(" }");
      }
      break;
    case EXTERNAL_identification::ALT_transfer__syntax:
      if(TTCN_Logger::VERBOSITY_COMPACT == TTCN_Logger::get_matching_verbosity()){
        TTCN_Logger::log_logmatch_info(".transfer_syntax");
        single_value.field_transfer__syntax->log_match(match_value.transfer__syntax());
      }else{
      TTCN_Logger::log_event_str("{ transfer_syntax := ");
      single_value.field_transfer__syntax->log_match(match_value.transfer__syntax());
      TTCN_Logger::log_event_str(" }");
      }
      break;
    case EXTERNAL_identification::ALT_fixed:
      if(TTCN_Logger::VERBOSITY_COMPACT == TTCN_Logger::get_matching_verbosity()){
        TTCN_Logger::log_logmatch_info(".fixed");
        single_value.field_fixed->log_match(match_value.fixed());
      }else{
      TTCN_Logger::log_event_str("{ fixed := ");
      single_value.field_fixed->log_match(match_value.fixed());
      TTCN_Logger::log_event_str(" }");
      }
      break;
    default:
      TTCN_Logger::print_logmatch_buffer();
      TTCN_Logger::log_event_str("<invalid selector>");
      break;
    }
  } else {
    TTCN_Logger::print_logmatch_buffer();
    match_value.log();
    TTCN_Logger::log_event_str(" with ");
    log();
    if (match(match_value)) TTCN_Logger::log_event_str(" matched");
    else TTCN_Logger::log_event_str(" unmatched");
  }
}

void EXTERNAL_identification_template::encode_text(Text_Buf& text_buf) const
{
  encode_text_base(text_buf);
  switch (template_selection) {
  case SPECIFIC_VALUE:
    text_buf.push_int(single_value.union_selection);
    switch (single_value.union_selection) {
    case EXTERNAL_identification::ALT_syntaxes:
      single_value.field_syntaxes->encode_text(text_buf);
      break;
    case EXTERNAL_identification::ALT_syntax:
      single_value.field_syntax->encode_text(text_buf);
      break;
    case EXTERNAL_identification::ALT_presentation__context__id:
      single_value.field_presentation__context__id->encode_text(text_buf);
      break;
    case EXTERNAL_identification::ALT_context__negotiation:
      single_value.field_context__negotiation->encode_text(text_buf);
      break;
    case EXTERNAL_identification::ALT_transfer__syntax:
      single_value.field_transfer__syntax->encode_text(text_buf);
      break;
    case EXTERNAL_identification::ALT_fixed:
      single_value.field_fixed->encode_text(text_buf);
      break;
    default:
      TTCN_error("Internal error: Invalid selector in a specific value when encoding a template of union type EXTERNAL.identification.");
    }
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    text_buf.push_int(value_list.n_values);
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count].encode_text(text_buf);
    break;
  default:
    TTCN_error("Text encoder: Encoding an uninitialized template of type EXTERNAL.identification.");
  }
}

void EXTERNAL_identification_template::decode_text(Text_Buf& text_buf)
{
  clean_up();
  decode_text_base(text_buf);
  switch (template_selection) {
  case SPECIFIC_VALUE:
    {
      single_value.union_selection = EXTERNAL_identification::UNBOUND_VALUE;
      EXTERNAL_identification::union_selection_type new_selection = (EXTERNAL_identification::union_selection_type)text_buf.pull_int().get_val();
      switch (new_selection) {
      case EXTERNAL_identification::ALT_syntaxes:
        single_value.field_syntaxes = new EXTERNAL_identification_syntaxes_template;
        single_value.field_syntaxes->decode_text(text_buf);
        break;
      case EXTERNAL_identification::ALT_syntax:
        single_value.field_syntax = new OBJID_template;
        single_value.field_syntax->decode_text(text_buf);
        break;
      case EXTERNAL_identification::ALT_presentation__context__id:
        single_value.field_presentation__context__id = new INTEGER_template;
        single_value.field_presentation__context__id->decode_text(text_buf);
        break;
      case EXTERNAL_identification::ALT_context__negotiation:
        single_value.field_context__negotiation = new EXTERNAL_identification_context__negotiation_template;
        single_value.field_context__negotiation->decode_text(text_buf);
        break;
      case EXTERNAL_identification::ALT_transfer__syntax:
        single_value.field_transfer__syntax = new OBJID_template;
        single_value.field_transfer__syntax->decode_text(text_buf);
        break;
      case EXTERNAL_identification::ALT_fixed:
        single_value.field_fixed = new ASN_NULL_template;
        single_value.field_fixed->decode_text(text_buf);
        break;
      default:
        TTCN_error("Text decoder: Unrecognized union selector was received for a template of type EXTERNAL.identification.");
      }
      single_value.union_selection = new_selection;
    }
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = text_buf.pull_int().get_val();
    value_list.list_value = new EXTERNAL_identification_template[value_list.n_values];
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count].decode_text(text_buf);
    break;
  default:
    TTCN_error("Text decoder: Unrecognized selector was received in a template of type EXTERNAL.identification.");
  }
}

boolean EXTERNAL_identification_template::is_present(boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;
  return !match_omit(legacy);
}

boolean EXTERNAL_identification_template::match_omit(boolean legacy /* = FALSE */) const
{
  if (is_ifpresent) return TRUE;
  switch (template_selection) {
  case OMIT_VALUE:
  case ANY_OR_OMIT:
    return TRUE;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    if (legacy) {
      for (unsigned int i=0; i<value_list.n_values; i++)
        if (value_list.list_value[i].match_omit())
          return template_selection==VALUE_LIST;
      return template_selection==COMPLEMENTED_LIST;
    } // else fall through
  default:
    return FALSE;
  }
  return FALSE;
}

#ifndef TITAN_RUNTIME_2
void EXTERNAL_identification_template::check_restriction(template_res t_res, const char* t_name,
                                                         boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return;
  switch ((t_name&&(t_res==TR_VALUE))?TR_OMIT:t_res) {
  case TR_VALUE:
    if (!is_ifpresent && template_selection==SPECIFIC_VALUE) return;
    break;
  case TR_OMIT:
    if (!is_ifpresent && (template_selection==OMIT_VALUE ||
        template_selection==SPECIFIC_VALUE)) return;
    break;
  case TR_PRESENT:
    if (!match_omit(legacy)) return;
    break;
  default:
    return;
  }
  TTCN_error("Restriction `%s' on template of type %s violated.",
             get_res_name(t_res), t_name ? t_name : "EXTERNAL.identification");
}
#endif

EXTERNAL_identification_syntaxes::EXTERNAL_identification_syntaxes()
{
}

EXTERNAL_identification_syntaxes::EXTERNAL_identification_syntaxes(const OBJID& par_abstract,
                                                                   const OBJID& par_transfer)
  : field_abstract(par_abstract),
    field_transfer(par_transfer)
{
}

boolean EXTERNAL_identification_syntaxes::operator==(const EXTERNAL_identification_syntaxes& other_value) const
{
  return field_abstract==other_value.field_abstract
    && field_transfer==other_value.field_transfer;
}

int EXTERNAL_identification_syntaxes::size_of() const
{
  int ret_val = 2;
  return ret_val;
}

void EXTERNAL_identification_syntaxes::log() const
{
  TTCN_Logger::log_event_str("{ abstract := ");
  field_abstract.log();
  TTCN_Logger::log_event_str(", transfer := ");
  field_transfer.log();
  TTCN_Logger::log_event_str(" }");
}

boolean EXTERNAL_identification_syntaxes::is_bound() const
{
  if(field_abstract.is_bound()) return TRUE;
  if(field_transfer.is_bound()) return TRUE;
  return FALSE;
}

boolean EXTERNAL_identification_syntaxes::is_value() const
{
  if(!field_abstract.is_value()) return FALSE;
  if(!field_transfer.is_value()) return FALSE;
  return TRUE;
}

void EXTERNAL_identification_syntaxes::clean_up()
{
  field_abstract.clean_up();
  field_transfer.clean_up();
}

void EXTERNAL_identification_syntaxes::set_param(Module_Param& param)
{
  param.basic_check(Module_Param::BC_VALUE, "record value");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  switch (mp->get_type()) {
  case Module_Param::MP_Value_List:
    if (mp->get_size()==0) return;
    if (2!=mp->get_size()) {
      param.error("record value of type EXTERNAL.identification.syntaxes has 2 fields but list value has %d fields", (int)mp->get_size());
    }
    if (mp->get_elem(0)->get_type()!=Module_Param::MP_NotUsed) abstract().set_param(*mp->get_elem(0));
    if (mp->get_elem(1)->get_type()!=Module_Param::MP_NotUsed) transfer().set_param(*mp->get_elem(1));
    break;
  case Module_Param::MP_Assignment_List: {
    Vector<bool> value_used(mp->get_size());
    value_used.resize(mp->get_size(), FALSE);
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "abstract")) {
        abstract().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "transfer")) {
        transfer().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) if (!value_used[val_idx]) {
      mp->get_elem(val_idx)->error("Non existent field name in type EXTERNAL.identification.syntaxes: %s", mp->get_elem(val_idx)->get_id()->get_name());
      break;
    }
  } break;
  default:
    param.type_error("record value", "EXTERNAL.identification.syntaxes");
  }
}

#ifdef TITAN_RUNTIME_2
Module_Param* EXTERNAL_identification_syntaxes::get_param(Module_Param_Name& param_name) const
{
  if (!is_bound()) {
    return new Module_Param_Unbound();
  }
  Module_Param* mp_field_abstract = field_abstract.get_param(param_name);
  mp_field_abstract->set_id(new Module_Param_FieldName(mcopystr("abstract")));
  Module_Param* mp_field_transfer = field_transfer.get_param(param_name);
  mp_field_transfer->set_id(new Module_Param_FieldName(mcopystr("transfer")));
  Module_Param_Assignment_List* mp = new Module_Param_Assignment_List();
  mp->add_elem(mp_field_abstract);
  mp->add_elem(mp_field_transfer);
  return mp;
}
#endif

void EXTERNAL_identification_syntaxes::encode_text(Text_Buf& text_buf) const
{
  field_abstract.encode_text(text_buf);
  field_transfer.encode_text(text_buf);
}

void EXTERNAL_identification_syntaxes::decode_text(Text_Buf& text_buf)
{
  field_abstract.decode_text(text_buf);
  field_transfer.decode_text(text_buf);
}

struct EXTERNAL_identification_syntaxes_template::single_value_struct {
  OBJID_template field_abstract;
  OBJID_template field_transfer;
};

void EXTERNAL_identification_syntaxes_template::set_param(Module_Param& param)
{
  param.basic_check(Module_Param::BC_TEMPLATE, "record template");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  switch (mp->get_type()) {
  case Module_Param::MP_Omit:
    *this = OMIT_VALUE;
    break;
  case Module_Param::MP_Any:
    *this = ANY_VALUE;
    break;
  case Module_Param::MP_AnyOrNone:
    *this = ANY_OR_OMIT;
    break;
  case Module_Param::MP_List_Template:
  case Module_Param::MP_ComplementList_Template: {
    EXTERNAL_identification_syntaxes_template temp;
    temp.set_type(mp->get_type() == Module_Param::MP_List_Template ?
      VALUE_LIST : COMPLEMENTED_LIST, mp->get_size());
    for (size_t i=0; i<mp->get_size(); i++) {
      temp.list_item(i).set_param(*mp->get_elem(i));
    }
    *this = temp;
    break; }
  case Module_Param::MP_Value_List:
    if (mp->get_size()==0) break;
    if (2!=mp->get_size()) {
      param.error("record template of type EXTERNAL.identification.syntaxes has 2 fields but list value has %d fields", (int)mp->get_size());
    }
    if (mp->get_elem(0)->get_type()!=Module_Param::MP_NotUsed) abstract().set_param(*mp->get_elem(0));
    if (mp->get_elem(1)->get_type()!=Module_Param::MP_NotUsed) transfer().set_param(*mp->get_elem(1));
    break;
  case Module_Param::MP_Assignment_List: {
    Vector<bool> value_used(mp->get_size());
    value_used.resize(mp->get_size(), FALSE);
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "abstract")) {
        abstract().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "transfer")) {
        transfer().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) if (!value_used[val_idx]) {
      mp->get_elem(val_idx)->error("Non existent field name in type EXTERNAL.identification.syntaxes: %s", mp->get_elem(val_idx)->get_id()->get_name());
      break;
    }
  } break;
  default:
    param.type_error("record template", "EXTERNAL.identification.syntaxes");
  }
  is_ifpresent = param.get_ifpresent() || mp->get_ifpresent();
}

#ifdef TITAN_RUNTIME_2
Module_Param* EXTERNAL_identification_syntaxes_template::get_param(Module_Param_Name& param_name) const
{
  Module_Param* mp = NULL;
  switch (template_selection) {
  case UNINITIALIZED_TEMPLATE:
    mp = new Module_Param_Unbound();
    break;
  case OMIT_VALUE:
    mp = new Module_Param_Omit();
    break;
  case ANY_VALUE:
    mp = new Module_Param_Any();
    break;
  case ANY_OR_OMIT:
    mp = new Module_Param_AnyOrNone();
    break;
  case SPECIFIC_VALUE: {
    Module_Param* mp_field_abstract = single_value->field_abstract.get_param(param_name);
    mp_field_abstract->set_id(new Module_Param_FieldName(mcopystr("abstract")));
    Module_Param* mp_field_transfer = single_value->field_transfer.get_param(param_name);
    mp_field_transfer->set_id(new Module_Param_FieldName(mcopystr("transfer")));
    mp = new Module_Param_Assignment_List();
    mp->add_elem(mp_field_abstract);
    mp->add_elem(mp_field_transfer);
    break; }
  case VALUE_LIST:
  case COMPLEMENTED_LIST: {
    if (template_selection == VALUE_LIST) {
      mp = new Module_Param_List_Template();
    }
    else {
      mp = new Module_Param_ComplementList_Template();
    }
    for (size_t i = 0; i < value_list.n_values; ++i) {
      mp->add_elem(value_list.list_value[i].get_param(param_name));
    }
    break; }
  default:
    TTCN_error("Referencing an uninitialized/unsupported template of type EXTERNAL.identification.syntaxtes .");
    break;
  }
  if (is_ifpresent) {
    mp->set_ifpresent();
  }
  return mp;
}
#endif

void EXTERNAL_identification_syntaxes_template::clean_up()
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    delete single_value;
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    delete [] value_list.list_value;
    break;
  default:
    break;
  }
  template_selection = UNINITIALIZED_TEMPLATE;
}

void EXTERNAL_identification_syntaxes_template::set_specific()
{
  if (template_selection != SPECIFIC_VALUE) {
    template_sel old_selection = template_selection;
    clean_up();
    single_value = new single_value_struct;
    set_selection(SPECIFIC_VALUE);
    if (old_selection == ANY_VALUE || old_selection == ANY_OR_OMIT) {
      single_value->field_abstract = ANY_VALUE;
      single_value->field_transfer = ANY_VALUE;
    }
  }
}

void EXTERNAL_identification_syntaxes_template::copy_value(const EXTERNAL_identification_syntaxes& other_value)
{
  single_value = new single_value_struct;
  single_value->field_abstract = other_value.abstract();
  single_value->field_transfer = other_value.transfer();
  set_selection(SPECIFIC_VALUE);
}

void EXTERNAL_identification_syntaxes_template::copy_template(const EXTERNAL_identification_syntaxes_template& other_value)
{
  switch (other_value.template_selection) {
  case SPECIFIC_VALUE:
    single_value = new single_value_struct(*other_value.single_value);
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = other_value.value_list.n_values;
    value_list.list_value = new EXTERNAL_identification_syntaxes_template[value_list.n_values];
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count].copy_template(other_value.value_list.list_value[list_count]);
    break;
  default:
    TTCN_error("Copying an uninitialized/unsupported template of type EXTERNAL.identification.syntaxes.");
  }
  set_selection(other_value);
}

EXTERNAL_identification_syntaxes_template::EXTERNAL_identification_syntaxes_template()
{
}

EXTERNAL_identification_syntaxes_template::EXTERNAL_identification_syntaxes_template(template_sel other_value)
  : Base_Template(other_value)
{
  check_single_selection(other_value);
}

EXTERNAL_identification_syntaxes_template::EXTERNAL_identification_syntaxes_template(const EXTERNAL_identification_syntaxes& other_value)
{
  copy_value(other_value);
}

EXTERNAL_identification_syntaxes_template::EXTERNAL_identification_syntaxes_template(const OPTIONAL<EXTERNAL_identification_syntaxes>& other_value)
{
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    copy_value((const EXTERNAL_identification_syntaxes&)other_value);
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Creating a template of type EXTERNAL.identification.syntaxes from an unbound optional field.");
  }
}

EXTERNAL_identification_syntaxes_template::EXTERNAL_identification_syntaxes_template(const EXTERNAL_identification_syntaxes_template& other_value)
: Base_Template()
{
  copy_template(other_value);
}

EXTERNAL_identification_syntaxes_template::~EXTERNAL_identification_syntaxes_template()
{
  clean_up();
}

EXTERNAL_identification_syntaxes_template& EXTERNAL_identification_syntaxes_template::operator=(template_sel other_value)
{
  check_single_selection(other_value);
  clean_up();
  set_selection(other_value);
  return *this;
}

EXTERNAL_identification_syntaxes_template& EXTERNAL_identification_syntaxes_template::operator=(const EXTERNAL_identification_syntaxes& other_value)
{
  clean_up();
  copy_value(other_value);
  return *this;
}

EXTERNAL_identification_syntaxes_template& EXTERNAL_identification_syntaxes_template::operator=(const OPTIONAL<EXTERNAL_identification_syntaxes>& other_value)
{
  clean_up();
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    copy_value((const EXTERNAL_identification_syntaxes&)other_value);
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Assignment of an unbound optional field to a template of type EXTERNAL.identification.syntaxes.");
  }
  return *this;
}

EXTERNAL_identification_syntaxes_template& EXTERNAL_identification_syntaxes_template::operator=(const EXTERNAL_identification_syntaxes_template& other_value)
{
  if (&other_value != this) {
    clean_up();
    copy_template(other_value);
  }
  return *this;
}

boolean EXTERNAL_identification_syntaxes_template::match(const EXTERNAL_identification_syntaxes& other_value,
                                                         boolean /* legacy */) const
{
  switch (template_selection) {
  case ANY_VALUE:
  case ANY_OR_OMIT:
    return TRUE;
  case OMIT_VALUE:
    return FALSE;
  case SPECIFIC_VALUE:
    if (!other_value.abstract().is_bound()) return FALSE;
    if (!single_value->field_abstract.match(other_value.abstract())) return FALSE;
    if (!other_value.transfer().is_bound()) return FALSE;
    if (!single_value->field_transfer.match(other_value.transfer())) return FALSE;
    return TRUE;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      if (value_list.list_value[list_count].match(other_value)) return template_selection == VALUE_LIST;
    return template_selection == COMPLEMENTED_LIST;
  default:
    TTCN_error("Matching an uninitialized/unsupported template of type EXTERNAL.identification.syntaxes.");
  }
  return FALSE;
}

EXTERNAL_identification_syntaxes EXTERNAL_identification_syntaxes_template::valueof() const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Performing a valueof or send operation on a non-specific template of type EXTERNAL.identification.syntaxes.");
  EXTERNAL_identification_syntaxes ret_val;
  ret_val.abstract() = single_value->field_abstract.valueof();
  ret_val.transfer() = single_value->field_transfer.valueof();
  return ret_val;
}

void EXTERNAL_identification_syntaxes_template::set_type(template_sel template_type, unsigned int list_length)
{
  if (template_type != VALUE_LIST && template_type != COMPLEMENTED_LIST)
    TTCN_error("Setting an invalid list for a template of type EXTERNAL.identification.syntaxes.");
  clean_up();
  set_selection(template_type);
  value_list.n_values = list_length;
  value_list.list_value = new EXTERNAL_identification_syntaxes_template[list_length];
}

EXTERNAL_identification_syntaxes_template& EXTERNAL_identification_syntaxes_template::list_item(unsigned int list_index) const
{
  if (template_selection != VALUE_LIST && template_selection != COMPLEMENTED_LIST)
    TTCN_error("Accessing a list element of a non-list template of type EXTERNAL.identification.syntaxes.");
  if (list_index >= value_list.n_values)
    TTCN_error("Index overflow in a value list template of type EXTERNAL.identification.syntaxes.");
  return value_list.list_value[list_index];
}

OBJID_template& EXTERNAL_identification_syntaxes_template::abstract()
{
  set_specific();
  return single_value->field_abstract;
}

const OBJID_template& EXTERNAL_identification_syntaxes_template::abstract() const
{
  if (template_selection != SPECIFIC_VALUE)
    TTCN_error("Accessing field abstract of a non-specific template of type EXTERNAL.identification.syntaxes.");
  return single_value->field_abstract;
}

OBJID_template& EXTERNAL_identification_syntaxes_template::transfer()
{
  set_specific();
  return single_value->field_transfer;
}

const OBJID_template& EXTERNAL_identification_syntaxes_template::transfer() const
{
  if (template_selection != SPECIFIC_VALUE)
    TTCN_error("Accessing field transfer of a non-specific template of type EXTERNAL.identification.syntaxes.");
  return single_value->field_transfer;
}

int EXTERNAL_identification_syntaxes_template::size_of() const
{
  switch (template_selection)
    {
    case SPECIFIC_VALUE:
      {
        int ret_val = 2;
        return ret_val;
      }
    case VALUE_LIST:
      {
        if (value_list.n_values<1)
          TTCN_error("Internal error: Performing sizeof() operation on a template of type EXTERNAL.identification.syntaxes containing an empty list.");
        int item_size = value_list.list_value[0].size_of();
        for (unsigned int i = 1; i < value_list.n_values; i++)
          {
            if (value_list.list_value[i].size_of()!=item_size)
              TTCN_error("Performing sizeof() operation on a template of type EXTERNAL.identification.syntaxes containing a value list with different sizes.");
          }
        return item_size;
      }
    case OMIT_VALUE:
      TTCN_error("Performing sizeof() operation on a template of type EXTERNAL.identification.syntaxes containing omit value.");
    case ANY_VALUE:
    case ANY_OR_OMIT:
      TTCN_error("Performing sizeof() operation on a template of type EXTERNAL.identification.syntaxes containing */? value.");
    case COMPLEMENTED_LIST:
      TTCN_error("Performing sizeof() operation on a template of type EXTERNAL.identification.syntaxes containing complemented list.");
    default:
      TTCN_error("Performing sizeof() operation on an uninitialized/unsupported template of type EXTERNAL.identification.syntaxes.");
    }
  return 0;
}

void EXTERNAL_identification_syntaxes_template::log() const
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    TTCN_Logger::log_event_str("{ abstract := ");
    single_value->field_abstract.log();
    TTCN_Logger::log_event_str(", transfer := ");
    single_value->field_transfer.log();
    TTCN_Logger::log_event_str(" }");
    break;
  case COMPLEMENTED_LIST:
    TTCN_Logger::log_event_str("complement ");
    // no break
  case VALUE_LIST:
    TTCN_Logger::log_char('(');
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++) {
      if (list_count > 0) TTCN_Logger::log_event_str(", ");
      value_list.list_value[list_count].log();
    }
    TTCN_Logger::log_char(')');
    break;
  default:
    log_generic();
    break;
  }
  log_ifpresent();
}

void EXTERNAL_identification_syntaxes_template::log_match(const EXTERNAL_identification_syntaxes& match_value,
                                                          boolean /* legacy */) const
{
  if (template_selection == SPECIFIC_VALUE) {
    TTCN_Logger::log_event_str("{ abstract := ");
    single_value->field_abstract.log_match(match_value.abstract());
    TTCN_Logger::log_event_str(", transfer := ");
    single_value->field_transfer.log_match(match_value.transfer());
    TTCN_Logger::log_event_str(" }");
  } else {
    match_value.log();
    TTCN_Logger::log_event_str(" with ");
    log();
    if (match(match_value)) TTCN_Logger::log_event_str(" matched");
    else TTCN_Logger::log_event_str(" unmatched");
  }
}

void EXTERNAL_identification_syntaxes_template::encode_text(Text_Buf& text_buf) const
{
  encode_text_base(text_buf);
  switch (template_selection) {
  case SPECIFIC_VALUE:
    single_value->field_abstract.encode_text(text_buf);
    single_value->field_transfer.encode_text(text_buf);
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    text_buf.push_int(value_list.n_values);
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count].encode_text(text_buf);
    break;
  default:
    TTCN_error("Text encoder: Encoding an uninitialized/unsupported template of type EXTERNAL.identification.syntaxes.");
  }
}

void EXTERNAL_identification_syntaxes_template::decode_text(Text_Buf& text_buf)
{
  clean_up();
  decode_text_base(text_buf);
  switch (template_selection) {
  case SPECIFIC_VALUE:
    single_value = new single_value_struct;
    single_value->field_abstract.decode_text(text_buf);
    single_value->field_transfer.decode_text(text_buf);
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = text_buf.pull_int().get_val();
    value_list.list_value = new EXTERNAL_identification_syntaxes_template[value_list.n_values];
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count].decode_text(text_buf);
    break;
  default:
    TTCN_error("Text decoder: An unknown/unsupported selection was received in a template of type EXTERNAL.identification.syntaxes.");
  }
}

boolean EXTERNAL_identification_syntaxes_template::is_present(boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;
  return !match_omit(legacy);
}

boolean EXTERNAL_identification_syntaxes_template::match_omit(boolean legacy /* = FALSE */) const
{
  if (is_ifpresent) return TRUE;
  switch (template_selection) {
  case OMIT_VALUE:
  case ANY_OR_OMIT:
    return TRUE;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    if (legacy) {
      for (unsigned int i=0; i<value_list.n_values; i++)
        if (value_list.list_value[i].match_omit())
          return template_selection==VALUE_LIST;
      return template_selection==COMPLEMENTED_LIST;
    } // else fall through
  default:
    return FALSE;
  }
  return FALSE;
}

#ifndef TITAN_RUNTIME_2
void EXTERNAL_identification_syntaxes_template::check_restriction(template_res t_res, const char* t_name,
                                                                  boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return;
  switch ((t_name&&(t_res==TR_VALUE))?TR_OMIT:t_res) {
  case TR_VALUE:
    if (!is_ifpresent && template_selection==SPECIFIC_VALUE) return;
    break;
  case TR_OMIT:
    if (!is_ifpresent && (template_selection==OMIT_VALUE ||
        template_selection==SPECIFIC_VALUE)) return;
    break;
  case TR_PRESENT:
    if (!match_omit(legacy)) return;
    break;
  default:
    return;
  }
  TTCN_error("Restriction `%s' on template of type %s violated.",
             get_res_name(t_res), t_name ? t_name : "EXTERNAL.identification.syntaxes");
}
#endif

EXTERNAL_identification_context__negotiation::EXTERNAL_identification_context__negotiation()
{
}

EXTERNAL_identification_context__negotiation::EXTERNAL_identification_context__negotiation(const INTEGER& par_presentation__context__id,
                                                                                           const OBJID& par_transfer__syntax)
  : field_presentation__context__id(par_presentation__context__id),
    field_transfer__syntax(par_transfer__syntax)
{
}

boolean EXTERNAL_identification_context__negotiation::operator==(const EXTERNAL_identification_context__negotiation& other_value) const
{
  return field_presentation__context__id==other_value.field_presentation__context__id
    && field_transfer__syntax==other_value.field_transfer__syntax;
}

int EXTERNAL_identification_context__negotiation::size_of() const
{
  int ret_val = 2;
  return ret_val;
}

void EXTERNAL_identification_context__negotiation::log() const
{
  TTCN_Logger::log_event_str("{ presentation_context_id := ");
  field_presentation__context__id.log();
  TTCN_Logger::log_event_str(", transfer_syntax := ");
  field_transfer__syntax.log();
  TTCN_Logger::log_event_str(" }");
}

boolean EXTERNAL_identification_context__negotiation::is_bound() const
{
  if(field_presentation__context__id.is_bound()) return TRUE;
  if(field_transfer__syntax.is_bound()) return TRUE;
  return FALSE;
}

boolean EXTERNAL_identification_context__negotiation::is_value() const
{
  if(!field_presentation__context__id.is_value()) return FALSE;
  if(!field_transfer__syntax.is_value()) return FALSE;
  return TRUE;
}

void EXTERNAL_identification_context__negotiation::clean_up()
{
  field_presentation__context__id.clean_up();
  field_transfer__syntax.clean_up();
}

void EXTERNAL_identification_context__negotiation::set_param(Module_Param& param)
{
  param.basic_check(Module_Param::BC_VALUE, "record value");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  switch (mp->get_type()) {
  case Module_Param::MP_Value_List:
    if (mp->get_size()==0) return;
    if (2!=mp->get_size()) {
      param.error("record value of type EXTERNAL.identification.context-negotiation has 2 fields but list value has %d fields", (int)mp->get_size());
    }
    if (mp->get_elem(0)->get_type()!=Module_Param::MP_NotUsed) presentation__context__id().set_param(*mp->get_elem(0));
    if (mp->get_elem(1)->get_type()!=Module_Param::MP_NotUsed) transfer__syntax().set_param(*mp->get_elem(1));
    break;
  case Module_Param::MP_Assignment_List: {
    Vector<bool> value_used(mp->get_size());
    value_used.resize(mp->get_size(), FALSE);
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "presentation_context_id")) {
        presentation__context__id().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "transfer_syntax")) {
        transfer__syntax().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) if (!value_used[val_idx]) {
      mp->get_elem(val_idx)->error("Non existent field name in type EXTERNAL.identification.context-negotiation: %s", mp->get_elem(val_idx)->get_id()->get_name());
      break;
    }
  } break;
  default:
    param.type_error("record value", "EXTERNAL.identification.context-negotiation");
  }
}

#ifdef TITAN_RUNTIME_2
Module_Param* EXTERNAL_identification_context__negotiation::get_param(Module_Param_Name& param_name) const
{
  if (!is_bound()) {
    return new Module_Param_Unbound();
  }
  Module_Param* mp_field_presentation_context_id = field_presentation__context__id.get_param(param_name);
  mp_field_presentation_context_id->set_id(new Module_Param_FieldName(mcopystr("presentation_context_id")));
  Module_Param* mp_field_transfer_syntax = field_transfer__syntax.get_param(param_name);
  mp_field_transfer_syntax->set_id(new Module_Param_FieldName(mcopystr("transfer_syntax")));
  Module_Param_Assignment_List* mp = new Module_Param_Assignment_List();
  mp->add_elem(mp_field_presentation_context_id);
  mp->add_elem(mp_field_transfer_syntax);
  return mp;
}
#endif

void EXTERNAL_identification_context__negotiation::encode_text(Text_Buf& text_buf) const
{
  field_presentation__context__id.encode_text(text_buf);
  field_transfer__syntax.encode_text(text_buf);
}

void EXTERNAL_identification_context__negotiation::decode_text(Text_Buf& text_buf)
{
  field_presentation__context__id.decode_text(text_buf);
  field_transfer__syntax.decode_text(text_buf);
}

struct EXTERNAL_identification_context__negotiation_template::single_value_struct {
  INTEGER_template field_presentation__context__id;
  OBJID_template field_transfer__syntax;
};

void EXTERNAL_identification_context__negotiation_template::set_param(Module_Param& param)
{
  param.basic_check(Module_Param::BC_TEMPLATE, "record template");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  switch (mp->get_type()) {
  case Module_Param::MP_Omit:
    *this = OMIT_VALUE;
    break;
  case Module_Param::MP_Any:
    *this = ANY_VALUE;
    break;
  case Module_Param::MP_AnyOrNone:
    *this = ANY_OR_OMIT;
    break;
  case Module_Param::MP_List_Template:
  case Module_Param::MP_ComplementList_Template: {
    EXTERNAL_identification_context__negotiation_template temp;
    temp.set_type(mp->get_type() == Module_Param::MP_List_Template ?
      VALUE_LIST : COMPLEMENTED_LIST, mp->get_size());
    for (size_t i=0; i<mp->get_size(); i++) {
      temp.list_item(i).set_param(*mp->get_elem(i));
    }
    *this = temp;
    break; }
  case Module_Param::MP_Value_List:
    if (mp->get_size()==0) break;
    if (2!=mp->get_size()) {
      param.error("record template of type EXTERNAL.identification.context-negotiation has 2 fields but list value has %d fields", (int)mp->get_size());
    }
    if (mp->get_elem(0)->get_type()!=Module_Param::MP_NotUsed) presentation__context__id().set_param(*mp->get_elem(0));
    if (mp->get_elem(1)->get_type()!=Module_Param::MP_NotUsed) transfer__syntax().set_param(*mp->get_elem(1));
    break;
  case Module_Param::MP_Assignment_List: {
    Vector<bool> value_used(mp->get_size());
    value_used.resize(mp->get_size(), FALSE);
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "presentation_context_id")) {
        presentation__context__id().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "transfer_syntax")) {
        transfer__syntax().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) if (!value_used[val_idx]) {
      mp->get_elem(val_idx)->error("Non existent field name in type EXTERNAL.identification.context-negotiation: %s", mp->get_elem(val_idx)->get_id()->get_name());
      break;
    }
  } break;
  default:
    param.type_error("record template", "EXTERNAL.identification.context-negotiation");
  }
  is_ifpresent = param.get_ifpresent() || mp->get_ifpresent();
}

#ifdef TITAN_RUNTIME_2
Module_Param* EXTERNAL_identification_context__negotiation_template::get_param(Module_Param_Name& param_name) const
{
  Module_Param* mp = NULL;
  switch (template_selection) {
  case UNINITIALIZED_TEMPLATE:
    mp = new Module_Param_Unbound();
    break;
  case OMIT_VALUE:
    mp = new Module_Param_Omit();
    break;
  case ANY_VALUE:
    mp = new Module_Param_Any();
    break;
  case ANY_OR_OMIT:
    mp = new Module_Param_AnyOrNone();
    break;
  case SPECIFIC_VALUE: {
    Module_Param* mp_field_presentation_context_id = single_value->field_presentation__context__id.get_param(param_name);
    mp_field_presentation_context_id->set_id(new Module_Param_FieldName(mcopystr("presentation_context_id")));
    Module_Param* mp_field_transfer_syntax = single_value->field_transfer__syntax.get_param(param_name);
    mp_field_transfer_syntax->set_id(new Module_Param_FieldName(mcopystr("transfer_syntax")));
    mp = new Module_Param_Assignment_List();
    mp->add_elem(mp_field_presentation_context_id);
    mp->add_elem(mp_field_transfer_syntax);
    break; }
  case VALUE_LIST:
  case COMPLEMENTED_LIST: {
    if (template_selection == VALUE_LIST) {
      mp = new Module_Param_List_Template();
    }
    else {
      mp = new Module_Param_ComplementList_Template();
    }
    for (size_t i = 0; i < value_list.n_values; ++i) {
      mp->add_elem(value_list.list_value[i].get_param(param_name));
    }
    break; }
  default:
    TTCN_error("Referencing an uninitialized/unsupported template of type EXTERNAL.identification.context-negotiation.");
    break;
  }
  if (is_ifpresent) {
    mp->set_ifpresent();
  }
  return mp;
}
#endif

void EXTERNAL_identification_context__negotiation_template::clean_up()
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    delete single_value;
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    delete [] value_list.list_value;
    break;
  default:
    break;
  }
  template_selection = UNINITIALIZED_TEMPLATE;
}

void EXTERNAL_identification_context__negotiation_template::set_specific()
{
  if (template_selection != SPECIFIC_VALUE) {
    template_sel old_selection = template_selection;
    clean_up();
    single_value = new single_value_struct;
    set_selection(SPECIFIC_VALUE);
    if (old_selection == ANY_VALUE || old_selection == ANY_OR_OMIT) {
      single_value->field_presentation__context__id = ANY_VALUE;
      single_value->field_transfer__syntax = ANY_VALUE;
    }
  }
}

void EXTERNAL_identification_context__negotiation_template::copy_value(const EXTERNAL_identification_context__negotiation& other_value)
{
  single_value = new single_value_struct;
  single_value->field_presentation__context__id = other_value.presentation__context__id();
  single_value->field_transfer__syntax = other_value.transfer__syntax();
  set_selection(SPECIFIC_VALUE);
}

void EXTERNAL_identification_context__negotiation_template::copy_template(const EXTERNAL_identification_context__negotiation_template& other_value)
{
  switch (other_value.template_selection) {
  case SPECIFIC_VALUE:
    single_value = new single_value_struct(*other_value.single_value);
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = other_value.value_list.n_values;
    value_list.list_value = new EXTERNAL_identification_context__negotiation_template[value_list.n_values];
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count].copy_template(other_value.value_list.list_value[list_count]);
    break;
  default:
    TTCN_error("Copying an uninitialized/unsupported template of type EXTERNAL.identification.context-negotiation.");
  }
  set_selection(other_value);
}

EXTERNAL_identification_context__negotiation_template::EXTERNAL_identification_context__negotiation_template()
{
}

EXTERNAL_identification_context__negotiation_template::EXTERNAL_identification_context__negotiation_template(template_sel other_value)
  : Base_Template(other_value)
{
  check_single_selection(other_value);
}

EXTERNAL_identification_context__negotiation_template::EXTERNAL_identification_context__negotiation_template(const EXTERNAL_identification_context__negotiation& other_value)
{
  copy_value(other_value);
}

EXTERNAL_identification_context__negotiation_template::EXTERNAL_identification_context__negotiation_template(const OPTIONAL<EXTERNAL_identification_context__negotiation>& other_value)
{
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    copy_value((const EXTERNAL_identification_context__negotiation&)other_value);
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Creating a template of type EXTERNAL.identification.context-negotiation from an unbound optional field.");
  }
}

EXTERNAL_identification_context__negotiation_template::EXTERNAL_identification_context__negotiation_template(const EXTERNAL_identification_context__negotiation_template& other_value)
: Base_Template()
{
  copy_template(other_value);
}

EXTERNAL_identification_context__negotiation_template::~EXTERNAL_identification_context__negotiation_template()
{
  clean_up();
}

EXTERNAL_identification_context__negotiation_template& EXTERNAL_identification_context__negotiation_template::operator=(template_sel other_value)
{
  check_single_selection(other_value);
  clean_up();
  set_selection(other_value);
  return *this;
}

EXTERNAL_identification_context__negotiation_template& EXTERNAL_identification_context__negotiation_template::operator=(const EXTERNAL_identification_context__negotiation& other_value)
{
  clean_up();
  copy_value(other_value);
  return *this;
}

EXTERNAL_identification_context__negotiation_template& EXTERNAL_identification_context__negotiation_template::operator=(const OPTIONAL<EXTERNAL_identification_context__negotiation>& other_value)
{
  clean_up();
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    copy_value((const EXTERNAL_identification_context__negotiation&)other_value);
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Assignment of an unbound optional field to a template of type EXTERNAL.identification.context-negotiation.");
  }
  return *this;
}

EXTERNAL_identification_context__negotiation_template& EXTERNAL_identification_context__negotiation_template::operator=(const EXTERNAL_identification_context__negotiation_template& other_value)
{
  if (&other_value != this) {
    clean_up();
    copy_template(other_value);
  }
  return *this;
}

boolean EXTERNAL_identification_context__negotiation_template::match(const EXTERNAL_identification_context__negotiation& other_value,
                                                                     boolean /* legacy */) const
{
  switch (template_selection) {
  case ANY_VALUE:
  case ANY_OR_OMIT:
    return TRUE;
  case OMIT_VALUE:
    return FALSE;
  case SPECIFIC_VALUE:
    if (!other_value.presentation__context__id().is_bound()) return FALSE;
    if (!single_value->field_presentation__context__id.match(other_value.presentation__context__id())) return FALSE;
    if (!other_value.transfer__syntax().is_bound()) return FALSE;
    if (!single_value->field_transfer__syntax.match(other_value.transfer__syntax())) return FALSE;
    return TRUE;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      if (value_list.list_value[list_count].match(other_value)) return template_selection == VALUE_LIST;
    return template_selection == COMPLEMENTED_LIST;
  default:
    TTCN_error("Matching an uninitialized/unsupported template of type EXTERNAL.identification.context-negotiation.");
  }
  return FALSE;
}

EXTERNAL_identification_context__negotiation EXTERNAL_identification_context__negotiation_template::valueof() const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Performing a valueof or send operation on a non-specific template of type EXTERNAL.identification.context-negotiation.");
  EXTERNAL_identification_context__negotiation ret_val;
  ret_val.presentation__context__id() = single_value->field_presentation__context__id.valueof();
  ret_val.transfer__syntax() = single_value->field_transfer__syntax.valueof();
  return ret_val;
}

void EXTERNAL_identification_context__negotiation_template::set_type(template_sel template_type, unsigned int list_length)
{
  if (template_type != VALUE_LIST && template_type != COMPLEMENTED_LIST)
    TTCN_error("Setting an invalid list for a template of type EXTERNAL.identification.context-negotiation.");
  clean_up();
  set_selection(template_type);
  value_list.n_values = list_length;
  value_list.list_value = new EXTERNAL_identification_context__negotiation_template[list_length];
}

EXTERNAL_identification_context__negotiation_template& EXTERNAL_identification_context__negotiation_template::list_item(unsigned int list_index) const
{
  if (template_selection != VALUE_LIST && template_selection != COMPLEMENTED_LIST)
    TTCN_error("Accessing a list element of a non-list template of type EXTERNAL.identification.context-negotiation.");
  if (list_index >= value_list.n_values)
    TTCN_error("Index overflow in a value list template of type EXTERNAL.identification.context-negotiation.");
  return value_list.list_value[list_index];
}

INTEGER_template& EXTERNAL_identification_context__negotiation_template::presentation__context__id()
{
  set_specific();
  return single_value->field_presentation__context__id;
}

const INTEGER_template& EXTERNAL_identification_context__negotiation_template::presentation__context__id() const
{
  if (template_selection != SPECIFIC_VALUE)
    TTCN_error("Accessing field presentation_context_id of a non-specific template of type EXTERNAL.identification.context-negotiation.");
  return single_value->field_presentation__context__id;
}

OBJID_template& EXTERNAL_identification_context__negotiation_template::transfer__syntax()
{
  set_specific();
  return single_value->field_transfer__syntax;
}

const OBJID_template& EXTERNAL_identification_context__negotiation_template::transfer__syntax() const
{
  if (template_selection != SPECIFIC_VALUE)
    TTCN_error("Accessing field transfer_syntax of a non-specific template of type EXTERNAL.identification.context-negotiation.");
  return single_value->field_transfer__syntax;
}

int EXTERNAL_identification_context__negotiation_template::size_of() const
{
  switch (template_selection)
    {
    case SPECIFIC_VALUE:
      {
        int ret_val = 2;
        return ret_val;
      }
    case VALUE_LIST:
      {
        if (value_list.n_values<1)
          TTCN_error("Internal error: Performing sizeof() operation on a template of type EXTERNAL.identification.context-negotiation containing an empty list.");
        int item_size = value_list.list_value[0].size_of();
        for (unsigned int i = 1; i < value_list.n_values; i++)
          {
            if (value_list.list_value[i].size_of()!=item_size)
              TTCN_error("Performing sizeof() operation on a template of type EXTERNAL.identification.context-negotiation containing a value list with different sizes.");
          }
        return item_size;
      }
    case OMIT_VALUE:
      TTCN_error("Performing sizeof() operation on a template of type EXTERNAL.identification.context-negotiation containing omit value.");
    case ANY_VALUE:
    case ANY_OR_OMIT:
      TTCN_error("Performing sizeof() operation on a template of type EXTERNAL.identification.context-negotiation containing */? value.");
    case COMPLEMENTED_LIST:
      TTCN_error("Performing sizeof() operation on a template of type EXTERNAL.identification.context-negotiation containing complemented list.");
    default:
      TTCN_error("Performing sizeof() operation on an uninitialized/unsupported template of type EXTERNAL.identification.context-negotiation.");
    }
  return 0;
}

void EXTERNAL_identification_context__negotiation_template::log() const
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    TTCN_Logger::log_event_str("{ presentation_context_id := ");
    single_value->field_presentation__context__id.log();
    TTCN_Logger::log_event_str(", transfer_syntax := ");
    single_value->field_transfer__syntax.log();
    TTCN_Logger::log_event_str(" }");
    break;
  case COMPLEMENTED_LIST:
    TTCN_Logger::log_event_str("complement ");
    // no break
  case VALUE_LIST:
    TTCN_Logger::log_char('(');
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++) {
      if (list_count > 0) TTCN_Logger::log_event_str(", ");
      value_list.list_value[list_count].log();
    }
    TTCN_Logger::log_char(')');
    break;
  default:
    log_generic();
    break;
  }
  log_ifpresent();
}

void EXTERNAL_identification_context__negotiation_template::log_match(const EXTERNAL_identification_context__negotiation& match_value,
                                                                      boolean /* legacy */) const
{
  if (template_selection == SPECIFIC_VALUE) {
    TTCN_Logger::log_event_str("{ presentation_context_id := ");
    single_value->field_presentation__context__id.log_match(match_value.presentation__context__id());
    TTCN_Logger::log_event_str(", transfer_syntax := ");
    single_value->field_transfer__syntax.log_match(match_value.transfer__syntax());
    TTCN_Logger::log_event_str(" }");
  } else {
    match_value.log();
    TTCN_Logger::log_event_str(" with ");
    log();
    if (match(match_value)) TTCN_Logger::log_event_str(" matched");
    else TTCN_Logger::log_event_str(" unmatched");
  }
}

void EXTERNAL_identification_context__negotiation_template::encode_text(Text_Buf& text_buf) const
{
  encode_text_base(text_buf);
  switch (template_selection) {
  case SPECIFIC_VALUE:
    single_value->field_presentation__context__id.encode_text(text_buf);
    single_value->field_transfer__syntax.encode_text(text_buf);
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    text_buf.push_int(value_list.n_values);
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count].encode_text(text_buf);
    break;
  default:
    TTCN_error("Text encoder: Encoding an uninitialized/unsupported template of type EXTERNAL.identification.context-negotiation.");
  }
}

void EXTERNAL_identification_context__negotiation_template::decode_text(Text_Buf& text_buf)
{
  clean_up();
  decode_text_base(text_buf);
  switch (template_selection) {
  case SPECIFIC_VALUE:
    single_value = new single_value_struct;
    single_value->field_presentation__context__id.decode_text(text_buf);
    single_value->field_transfer__syntax.decode_text(text_buf);
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = text_buf.pull_int().get_val();
    value_list.list_value = new EXTERNAL_identification_context__negotiation_template[value_list.n_values];
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count].decode_text(text_buf);
    break;
  default:
    TTCN_error("Text decoder: An unknown/unsupported selection was received in a template of type EXTERNAL.identification.context-negotiation.");
  }
}

boolean EXTERNAL_identification_context__negotiation_template::is_present(boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;
  return !match_omit(legacy);
}

boolean EXTERNAL_identification_context__negotiation_template::match_omit(boolean legacy /* = FALSE */) const
{
  if (is_ifpresent) return TRUE;
  switch (template_selection) {
  case OMIT_VALUE:
  case ANY_OR_OMIT:
    return TRUE;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    if (legacy) {
      for (unsigned int i=0; i<value_list.n_values; i++)
        if (value_list.list_value[i].match_omit())
          return template_selection==VALUE_LIST;
      return template_selection==COMPLEMENTED_LIST;
    } // else fall through
  default:
    return FALSE;
  }
  return FALSE;
}

#ifndef TITAN_RUNTIME_2
void EXTERNAL_identification_context__negotiation_template::check_restriction(template_res t_res, const char* t_name,
                                                                              boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return;
  switch ((t_name&&(t_res==TR_VALUE))?TR_OMIT:t_res) {
  case TR_VALUE:
    if (!is_ifpresent && template_selection==SPECIFIC_VALUE) return;
    break;
  case TR_OMIT:
    if (!is_ifpresent && (template_selection==OMIT_VALUE ||
        template_selection==SPECIFIC_VALUE)) return;
    break;
  case TR_PRESENT:
    if (!match_omit(legacy)) return;
    break;
  default:
    return;
  }
  TTCN_error("Restriction `%s' on template of type %s violated.",
             get_res_name(t_res), t_name ? t_name : "EXTERNAL.identification.context-negotiation");
}
#endif

EXTERNAL::EXTERNAL()
{
}

EXTERNAL::EXTERNAL(const EXTERNAL_identification& par_identification,
                   const OPTIONAL<UNIVERSAL_CHARSTRING>& par_data__value__descriptor,
                   const OCTETSTRING& par_data__value)
  : field_identification(par_identification),
    field_data__value__descriptor(par_data__value__descriptor),
    field_data__value(par_data__value)
{
}

boolean EXTERNAL::operator==(const EXTERNAL& other_value) const
{
  return field_identification==other_value.field_identification
    && field_data__value__descriptor==other_value.field_data__value__descriptor
    && field_data__value==other_value.field_data__value;
}

int EXTERNAL::size_of() const
{
  int ret_val = 2;
  if (field_data__value__descriptor.ispresent()) ret_val++;
  return ret_val;
}

void EXTERNAL::log() const
{
  TTCN_Logger::log_event_str("{ identification := ");
  field_identification.log();
  TTCN_Logger::log_event_str(", data_value_descriptor := ");
  field_data__value__descriptor.log();
  TTCN_Logger::log_event_str(", data_value := ");
  field_data__value.log();
  TTCN_Logger::log_event_str(" }");
}

boolean EXTERNAL::is_bound() const
{
  if(field_identification.is_bound()) return TRUE;
  if(OPTIONAL_OMIT == field_data__value__descriptor.get_selection() || field_data__value__descriptor.is_bound()) return TRUE;
  if(field_data__value.is_bound()) return TRUE;
  return FALSE;
}

boolean EXTERNAL::is_value() const
{
  if(!field_identification.is_value()) return FALSE;
  if(OPTIONAL_OMIT != field_data__value__descriptor.get_selection() && !field_data__value__descriptor.is_value()) return FALSE;
  if(!field_data__value.is_value()) return FALSE;
  return TRUE;
}

void EXTERNAL::clean_up()
{
  field_identification.clean_up();
  field_data__value__descriptor.clean_up();
  field_data__value.clean_up();
}

void EXTERNAL::set_param(Module_Param& param)
{
  param.basic_check(Module_Param::BC_VALUE, "record value");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  switch (mp->get_type()) {
  case Module_Param::MP_Value_List:
    if (mp->get_size()==0) return;
    if (3!=mp->get_size()) {
      param.error("record value of type EXTERNAL has 3 fields but list value has %d fields", (int)mp->get_size());
    }
    if (mp->get_elem(0)->get_type()!=Module_Param::MP_NotUsed) identification().set_param(*mp->get_elem(0));
    if (mp->get_elem(1)->get_type()!=Module_Param::MP_NotUsed) data__value__descriptor().set_param(*mp->get_elem(1));
    if (mp->get_elem(2)->get_type()!=Module_Param::MP_NotUsed) data__value().set_param(*mp->get_elem(2));
    break;
  case Module_Param::MP_Assignment_List: {
    Vector<bool> value_used(mp->get_size());
    value_used.resize(mp->get_size(), FALSE);
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "identification")) {
        identification().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "data_value_descriptor")) {
        data__value__descriptor().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "data_value")) {
        data__value().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) if (!value_used[val_idx]) {
      mp->get_elem(val_idx)->error("Non existent field name in type EXTERNAL: %s", mp->get_elem(val_idx)->get_id()->get_name());
      break;
    }
  } break;
  default:
    param.type_error("record value", "EXTERNAL");
  }
}

#ifdef TITAN_RUNTIME_2
Module_Param* EXTERNAL::get_param(Module_Param_Name& param_name) const
{
  if (!is_bound()) {
    return new Module_Param_Unbound();
  }
  Module_Param* mp_field_identification = field_identification.get_param(param_name);
  mp_field_identification->set_id(new Module_Param_FieldName(mcopystr("identification")));
  Module_Param* mp_field_data_value_descriptor = field_data__value__descriptor.get_param(param_name);
  mp_field_data_value_descriptor->set_id(new Module_Param_FieldName(mcopystr("data_value_descriptor")));
  Module_Param* mp_field_data_value = field_data__value.get_param(param_name);
  mp_field_data_value->set_id(new Module_Param_FieldName(mcopystr("data_value")));
  Module_Param_Assignment_List* mp = new Module_Param_Assignment_List();
  mp->add_elem(mp_field_identification);
  mp->add_elem(mp_field_data_value_descriptor);
  mp->add_elem(mp_field_data_value);
  return mp;
}
#endif

void EXTERNAL::encode_text(Text_Buf& text_buf) const
{
  field_identification.encode_text(text_buf);
  field_data__value__descriptor.encode_text(text_buf);
  field_data__value.encode_text(text_buf);
}

void EXTERNAL::decode_text(Text_Buf& text_buf)
{
  field_identification.decode_text(text_buf);
  field_data__value__descriptor.decode_text(text_buf);
  field_data__value.decode_text(text_buf);
}

void EXTERNAL::encode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf, TTCN_EncDec::coding_t p_coding, ...) const
{
  va_list pvar;
  va_start(pvar, p_coding);
  switch(p_coding) {
  case TTCN_EncDec::CT_BER: {
    TTCN_EncDec_ErrorContext ec("While BER-encoding type '%s': ", p_td.name);
    unsigned BER_coding=va_arg(pvar, unsigned);
    BER_encode_chk_coding(BER_coding);
    ASN_BER_TLV_t *tlv=BER_encode_TLV(p_td, BER_coding);
    tlv->put_in_buffer(p_buf);
    ASN_BER_TLV_t::destruct(tlv);
    break;}
  case TTCN_EncDec::CT_RAW: {
    TTCN_EncDec_ErrorContext ec("While RAW-encoding type '%s': ", p_td.name);
    TTCN_EncDec_ErrorContext::error_internal
      ("No RAW descriptor available for type '%s'.", p_td.name);
    break;}
  case TTCN_EncDec::CT_TEXT: {
    TTCN_EncDec_ErrorContext ec("While TEXT-encoding type '%s': ", p_td.name);
    TTCN_EncDec_ErrorContext::error_internal
      ("No TEXT descriptor available for type '%s'.", p_td.name);
    break;}
  case TTCN_EncDec::CT_XER: {
    TTCN_EncDec_ErrorContext ec("While XER-encoding type '%s': ", p_td.name);
    unsigned XER_coding=va_arg(pvar, unsigned);
    XER_encode(*p_td.xer, p_buf, XER_coding, 0, 0, 0);
    break;}
  case TTCN_EncDec::CT_JSON: {
    TTCN_EncDec_ErrorContext ec("While JSON-encoding type '%s': ", p_td.name);
    TTCN_EncDec_ErrorContext::error_internal
      ("No JSON descriptor available for type '%s'.", p_td.name);
    break;}
  default:
    TTCN_error("Unknown coding method requested to encode type '%s'", p_td.name);
  }
  va_end(pvar);
}

void EXTERNAL::decode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf, TTCN_EncDec::coding_t p_coding, ...)
{
  va_list pvar;
  va_start(pvar, p_coding);
  switch(p_coding) {
  case TTCN_EncDec::CT_BER: {
    TTCN_EncDec_ErrorContext ec("While BER-decoding type '%s': ", p_td.name);
    unsigned L_form=va_arg(pvar, unsigned);
    ASN_BER_TLV_t tlv;
    BER_decode_str2TLV(p_buf, tlv, L_form);
    BER_decode_TLV(p_td, tlv, L_form);
    if(tlv.isComplete) p_buf.increase_pos(tlv.get_len());
    break;}
  case TTCN_EncDec::CT_RAW: {
    TTCN_EncDec_ErrorContext ec("While RAW-decoding type '%s': ", p_td.name);
    TTCN_EncDec_ErrorContext::error_internal
      ("No RAW descriptor available for type '%s'.", p_td.name);
    break;}
  case TTCN_EncDec::CT_TEXT: {
    TTCN_EncDec_ErrorContext ec("While TEXT-decoding type '%s': ", p_td.name);
    TTCN_EncDec_ErrorContext::error_internal
      ("No TEXT descriptor available for type '%s'.", p_td.name);
    break;}
  case TTCN_EncDec::CT_XER: {
    TTCN_EncDec_ErrorContext ec("While XER-decoding type '%s': ", p_td.name);
    unsigned XER_coding=va_arg(pvar, unsigned);
    XmlReaderWrap reader(p_buf);
    int success = reader.Read();
    for (; success==1; success=reader.Read()) {
      int type = reader.NodeType();
      if (type==XML_READER_TYPE_ELEMENT)
	break;
    }
    XER_decode(*p_td.xer, reader, XER_coding, XER_NONE, 0);
    size_t bytes = reader.ByteConsumed();
    p_buf.set_pos(bytes);
    break;}
  case TTCN_EncDec::CT_JSON: {
    TTCN_EncDec_ErrorContext ec("While JSON-decoding type '%s': ", p_td.name);
    TTCN_EncDec_ErrorContext::error_internal
      ("No JSON descriptor available for type '%s'.", p_td.name);
    break;}
  default:
    TTCN_error("Unknown coding method requested to decode type '%s'", p_td.name);
  }
  va_end(pvar);
}

struct EXTERNAL_template::single_value_struct {
  EXTERNAL_identification_template field_identification;
  UNIVERSAL_CHARSTRING_template field_data__value__descriptor;
  OCTETSTRING_template field_data__value;
};

void EXTERNAL_template::set_param(Module_Param& param)
{
  param.basic_check(Module_Param::BC_TEMPLATE, "record template");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  switch (mp->get_type()) {
  case Module_Param::MP_Omit:
    *this = OMIT_VALUE;
    break;
  case Module_Param::MP_Any:
    *this = ANY_VALUE;
    break;
  case Module_Param::MP_AnyOrNone:
    *this = ANY_OR_OMIT;
    break;
  case Module_Param::MP_List_Template:
  case Module_Param::MP_ComplementList_Template: {
    EXTERNAL_template temp;
    temp.set_type(mp->get_type() == Module_Param::MP_List_Template ?
      VALUE_LIST : COMPLEMENTED_LIST, mp->get_size());
    for (size_t i=0; i<mp->get_size(); i++) {
      temp.list_item(i).set_param(*mp->get_elem(i));
    }
    *this = temp;
    break; }
  case Module_Param::MP_Value_List:
    if (mp->get_size()==0) break;
    if (3!=mp->get_size()) {
      param.error("record template of type EXTERNAL has 3 fields but list value has %d fields", (int)mp->get_size());
    }
    if (mp->get_elem(0)->get_type()!=Module_Param::MP_NotUsed) identification().set_param(*mp->get_elem(0));
    if (mp->get_elem(1)->get_type()!=Module_Param::MP_NotUsed) data__value__descriptor().set_param(*mp->get_elem(1));
    if (mp->get_elem(2)->get_type()!=Module_Param::MP_NotUsed) data__value().set_param(*mp->get_elem(2));
    break;
  case Module_Param::MP_Assignment_List: {
    Vector<bool> value_used(mp->get_size());
    value_used.resize(mp->get_size(), FALSE);
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "identification")) {
        identification().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "data_value_descriptor")) {
        data__value__descriptor().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) {
      Module_Param* const curr_param = mp->get_elem(val_idx);
      if (!strcmp(curr_param->get_id()->get_name(), "data_value")) {
        data__value().set_param(*curr_param);
        value_used[val_idx]=TRUE;
      }
    }
    for (size_t val_idx=0; val_idx<mp->get_size(); val_idx++) if (!value_used[val_idx]) {
      mp->get_elem(val_idx)->error("Non existent field name in type EXTERNAL: %s", mp->get_elem(val_idx)->get_id()->get_name());
      break;
    }
  } break;
  default:
    param.type_error("record template", "EXTERNAL");
  }
  is_ifpresent = param.get_ifpresent() || mp->get_ifpresent();
}

#ifdef TITAN_RUNTIME_2
Module_Param* EXTERNAL_template::get_param(Module_Param_Name& param_name) const
{
  Module_Param* mp = NULL;
  switch (template_selection) {
  case UNINITIALIZED_TEMPLATE:
    mp = new Module_Param_Unbound();
    break;
  case OMIT_VALUE:
    mp = new Module_Param_Omit();
    break;
  case ANY_VALUE:
    mp = new Module_Param_Any();
    break;
  case ANY_OR_OMIT:
    mp = new Module_Param_AnyOrNone();
    break;
  case SPECIFIC_VALUE: {
    Module_Param* mp_field_identification = single_value->field_identification.get_param(param_name);
    mp_field_identification->set_id(new Module_Param_FieldName(mcopystr("identification")));
    Module_Param* mp_field_data_value_descriptor = single_value->field_data__value__descriptor.get_param(param_name);
    mp_field_data_value_descriptor->set_id(new Module_Param_FieldName(mcopystr("data_value_descriptor")));
    Module_Param* mp_field_string_value = single_value->field_data__value.get_param(param_name);
    mp_field_string_value->set_id(new Module_Param_FieldName(mcopystr("data_value")));
    mp = new Module_Param_Assignment_List();
    mp->add_elem(mp_field_identification);
    mp->add_elem(mp_field_data_value_descriptor);
    mp->add_elem(mp_field_string_value);
    break; }
  case VALUE_LIST:
  case COMPLEMENTED_LIST: {
    if (template_selection == VALUE_LIST) {
      mp = new Module_Param_List_Template();
    }
    else {
      mp = new Module_Param_ComplementList_Template();
    }
    for (size_t i = 0; i < value_list.n_values; ++i) {
      mp->add_elem(value_list.list_value[i].get_param(param_name));
    }
    break; }
  default:
    TTCN_error("Referencing an uninitialized/unsupported template of type EXTERNAL.");
    break;
  }
  if (is_ifpresent) {
    mp->set_ifpresent();
  }
  return mp;
}
#endif

void EXTERNAL_template::clean_up()
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    delete single_value;
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    delete [] value_list.list_value;
    break;
  default:
    break;
  }
  template_selection = UNINITIALIZED_TEMPLATE;
}

void EXTERNAL_template::set_specific()
{
  if (template_selection != SPECIFIC_VALUE) {
    template_sel old_selection = template_selection;
    clean_up();
    single_value = new single_value_struct;
    set_selection(SPECIFIC_VALUE);
    if (old_selection == ANY_VALUE || old_selection == ANY_OR_OMIT) {
      single_value->field_identification = ANY_VALUE;
      single_value->field_data__value__descriptor = ANY_OR_OMIT;
      single_value->field_data__value = ANY_VALUE;
    }
  }
}

void EXTERNAL_template::copy_value(const EXTERNAL& other_value)
{
  single_value = new single_value_struct;
  single_value->field_identification = other_value.identification();
  if (other_value.data__value__descriptor().ispresent()) single_value->field_data__value__descriptor = (const UNIVERSAL_CHARSTRING&)(other_value.data__value__descriptor());
  else single_value->field_data__value__descriptor = OMIT_VALUE;
  single_value->field_data__value = other_value.data__value();
  set_selection(SPECIFIC_VALUE);
}

void EXTERNAL_template::copy_template(const EXTERNAL_template& other_value)
{
  switch (other_value.template_selection) {
  case SPECIFIC_VALUE:
    single_value = new single_value_struct(*other_value.single_value);
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = other_value.value_list.n_values;
    value_list.list_value = new EXTERNAL_template[value_list.n_values];
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count].copy_template(other_value.value_list.list_value[list_count]);
    break;
  default:
    TTCN_error("Copying an uninitialized/unsupported template of type EXTERNAL.");
  }
  set_selection(other_value);
}

EXTERNAL_template::EXTERNAL_template()
{
}

EXTERNAL_template::EXTERNAL_template(template_sel other_value)
  : Base_Template(other_value)
{
  check_single_selection(other_value);
}

EXTERNAL_template::EXTERNAL_template(const EXTERNAL& other_value)
{
  copy_value(other_value);
}

EXTERNAL_template::EXTERNAL_template(const OPTIONAL<EXTERNAL>& other_value)
{
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    copy_value((const EXTERNAL&)other_value);
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Creating a template of type EXTERNAL from an unbound optional field.");
  }
}

EXTERNAL_template::EXTERNAL_template(const EXTERNAL_template& other_value)
: Base_Template()
{
  copy_template(other_value);
}

EXTERNAL_template::~EXTERNAL_template()
{
  clean_up();
}

EXTERNAL_template& EXTERNAL_template::operator=(template_sel other_value)
{
  check_single_selection(other_value);
  clean_up();
  set_selection(other_value);
  return *this;
}

EXTERNAL_template& EXTERNAL_template::operator=(const EXTERNAL& other_value)
{
  clean_up();
  copy_value(other_value);
  return *this;
}

EXTERNAL_template& EXTERNAL_template::operator=(const OPTIONAL<EXTERNAL>& other_value)
{
  clean_up();
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    copy_value((const EXTERNAL&)other_value);
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Assignment of an unbound optional field to a template of type EXTERNAL.");
  }
  return *this;
}

EXTERNAL_template& EXTERNAL_template::operator=(const EXTERNAL_template& other_value)
{
  if (&other_value != this) {
    clean_up();
    copy_template(other_value);
  }
  return *this;
}

boolean EXTERNAL_template::match(const EXTERNAL& other_value,
                                 boolean /* legacy */) const
{
  switch (template_selection) {
  case ANY_VALUE:
  case ANY_OR_OMIT:
    return TRUE;
  case OMIT_VALUE:
    return FALSE;
  case SPECIFIC_VALUE:
	  if (!other_value.identification().is_bound()) return FALSE;
	  if (!single_value->field_identification.match(other_value.identification())) return FALSE;
	  if (!other_value.data__value__descriptor().is_bound()) return FALSE;
	  if (other_value.data__value__descriptor().ispresent() ? !single_value->field_data__value__descriptor.match((const UNIVERSAL_CHARSTRING&)other_value.data__value__descriptor()) : !single_value->field_data__value__descriptor.match_omit()) return FALSE;
	  if (!other_value.data__value().is_bound()) return FALSE;
	  if (!single_value->field_data__value.match(other_value.data__value())) return FALSE;
    return TRUE;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      if (value_list.list_value[list_count].match(other_value)) return template_selection == VALUE_LIST;
    return template_selection == COMPLEMENTED_LIST;
  default:
    TTCN_error("Matching an uninitialized/unsupported template of type EXTERNAL.");
  }
  return FALSE;
}

EXTERNAL EXTERNAL_template::valueof() const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Performing a valueof or send operation on a non-specific template of type EXTERNAL.");
  EXTERNAL ret_val;
  ret_val.identification() = single_value->field_identification.valueof();
  if (single_value->field_data__value__descriptor.is_omit()) ret_val.data__value__descriptor() = OMIT_VALUE;
  else ret_val.data__value__descriptor() = single_value->field_data__value__descriptor.valueof();
  ret_val.data__value() = single_value->field_data__value.valueof();
  return ret_val;
}

void EXTERNAL_template::set_type(template_sel template_type, unsigned int list_length)
{
  if (template_type != VALUE_LIST && template_type != COMPLEMENTED_LIST)
    TTCN_error("Setting an invalid list for a template of type EXTERNAL.");
  clean_up();
  set_selection(template_type);
  value_list.n_values = list_length;
  value_list.list_value = new EXTERNAL_template[list_length];
}

EXTERNAL_template& EXTERNAL_template::list_item(unsigned int list_index) const
{
  if (template_selection != VALUE_LIST && template_selection != COMPLEMENTED_LIST)
    TTCN_error("Accessing a list element of a non-list template of type EXTERNAL.");
  if (list_index >= value_list.n_values)
    TTCN_error("Index overflow in a value list template of type EXTERNAL.");
  return value_list.list_value[list_index];
}

EXTERNAL_identification_template& EXTERNAL_template::identification()
{
  set_specific();
  return single_value->field_identification;
}

const EXTERNAL_identification_template& EXTERNAL_template::identification() const
{
  if (template_selection != SPECIFIC_VALUE)
    TTCN_error("Accessing field identification of a non-specific template of type EXTERNAL.");
  return single_value->field_identification;
}

UNIVERSAL_CHARSTRING_template& EXTERNAL_template::data__value__descriptor()
{
  set_specific();
  return single_value->field_data__value__descriptor;
}

const UNIVERSAL_CHARSTRING_template& EXTERNAL_template::data__value__descriptor() const
{
  if (template_selection != SPECIFIC_VALUE)
    TTCN_error("Accessing field data_value_descriptor of a non-specific template of type EXTERNAL.");
  return single_value->field_data__value__descriptor;
}

OCTETSTRING_template& EXTERNAL_template::data__value()
{
  set_specific();
  return single_value->field_data__value;
}

const OCTETSTRING_template& EXTERNAL_template::data__value() const
{
  if (template_selection != SPECIFIC_VALUE)
    TTCN_error("Accessing field data_value of a non-specific template of type EXTERNAL.");
  return single_value->field_data__value;
}

int EXTERNAL_template::size_of() const
{
  switch (template_selection)
    {
    case SPECIFIC_VALUE:
      {
        int ret_val = 2;
        if (single_value->field_data__value__descriptor.is_present()) ret_val++;
        return ret_val;
      }
    case VALUE_LIST:
      {
        if (value_list.n_values<1)
          TTCN_error("Internal error: Performing sizeof() operation on a template of type EXTERNAL containing an empty list.");
        int item_size = value_list.list_value[0].size_of();
        for (unsigned int i = 1; i < value_list.n_values; i++)
          {
            if (value_list.list_value[i].size_of()!=item_size)
              TTCN_error("Performing sizeof() operation on a template of type EXTERNAL containing a value list with different sizes.");
          }
        return item_size;
      }
    case OMIT_VALUE:
      TTCN_error("Performing sizeof() operation on a template of type EXTERNAL containing omit value.");
    case ANY_VALUE:
    case ANY_OR_OMIT:
      TTCN_error("Performing sizeof() operation on a template of type EXTERNAL containing */? value.");
    case COMPLEMENTED_LIST:
      TTCN_error("Performing sizeof() operation on a template of type EXTERNAL containing complemented list.");
    default:
      TTCN_error("Performing sizeof() operation on an uninitialized/unsupported template of type EXTERNAL.");
    }
  return 0;
}

void EXTERNAL_template::log() const
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    TTCN_Logger::log_event_str("{ identification := ");
    single_value->field_identification.log();
    TTCN_Logger::log_event_str(", data_value_descriptor := ");
    single_value->field_data__value__descriptor.log();
    TTCN_Logger::log_event_str(", data_value := ");
    single_value->field_data__value.log();
    TTCN_Logger::log_event_str(" }");
    break;
  case COMPLEMENTED_LIST:
    TTCN_Logger::log_event_str("complement ");
    // no break
  case VALUE_LIST:
    TTCN_Logger::log_char('(');
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++) {
      if (list_count > 0) TTCN_Logger::log_event_str(", ");
      value_list.list_value[list_count].log();
    }
    TTCN_Logger::log_char(')');
    break;
  default:
    log_generic();
    break;
  }
  log_ifpresent();
}

void EXTERNAL_template::log_match(const EXTERNAL& match_value,
                                  boolean /* legacy */) const
{
  if (template_selection == SPECIFIC_VALUE) {
    TTCN_Logger::log_event_str("{ identification := ");
    single_value->field_identification.log_match(match_value.identification());
    TTCN_Logger::log_event_str(", data_value_descriptor := ");
    if (match_value.data__value__descriptor().ispresent()) single_value->field_data__value__descriptor.log_match(match_value.data__value__descriptor());
    else {
      single_value->field_data__value__descriptor.log();
      if (single_value->field_data__value__descriptor.match_omit()) TTCN_Logger::log_event_str(" matched");
      else TTCN_Logger::log_event_str(" unmatched");
    }
    TTCN_Logger::log_event_str(", data_value := ");
    single_value->field_data__value.log_match(match_value.data__value());
    TTCN_Logger::log_event_str(" }");
  } else {
    match_value.log();
    TTCN_Logger::log_event_str(" with ");
    log();
    if (match(match_value)) TTCN_Logger::log_event_str(" matched");
    else TTCN_Logger::log_event_str(" unmatched");
  }
}

void EXTERNAL_template::encode_text(Text_Buf& text_buf) const
{
  encode_text_base(text_buf);
  switch (template_selection) {
  case SPECIFIC_VALUE:
    single_value->field_identification.encode_text(text_buf);
    single_value->field_data__value__descriptor.encode_text(text_buf);
    single_value->field_data__value.encode_text(text_buf);
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    text_buf.push_int(value_list.n_values);
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count].encode_text(text_buf);
    break;
  default:
    TTCN_error("Text encoder: Encoding an uninitialized/unsupported template of type EXTERNAL.");
  }
}

void EXTERNAL_template::decode_text(Text_Buf& text_buf)
{
  clean_up();
  decode_text_base(text_buf);
  switch (template_selection) {
  case SPECIFIC_VALUE:
    single_value = new single_value_struct;
    single_value->field_identification.decode_text(text_buf);
    single_value->field_data__value__descriptor.decode_text(text_buf);
    single_value->field_data__value.decode_text(text_buf);
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = text_buf.pull_int().get_val();
    value_list.list_value = new EXTERNAL_template[value_list.n_values];
    for (unsigned int list_count = 0; list_count < value_list.n_values; list_count++)
      value_list.list_value[list_count].decode_text(text_buf);
    break;
  default:
    TTCN_error("Text decoder: An unknown/unsupported selection was received in a template of type EXTERNAL.");
  }
}

boolean EXTERNAL_template::is_present(boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;
  return !match_omit(legacy);
}

boolean EXTERNAL_template::match_omit(boolean legacy /* = FALSE */) const
{
  if (is_ifpresent) return TRUE;
  switch (template_selection) {
  case OMIT_VALUE:
  case ANY_OR_OMIT:
    return TRUE;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    if (legacy) {
      // legacy behavior: 'omit' can appear in the value/complement list
      for (unsigned int i=0; i<value_list.n_values; i++)
        if (value_list.list_value[i].match_omit())
          return template_selection==VALUE_LIST;
      return template_selection==COMPLEMENTED_LIST;
    }
    // else fall through
  default:
    return FALSE;
  }
  return FALSE;
}

#ifndef TITAN_RUNTIME_2
void EXTERNAL_template::check_restriction(template_res t_res, const char* t_name,
                                          boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return;
  switch ((t_name&&(t_res==TR_VALUE))?TR_OMIT:t_res) {
  case TR_VALUE:
    if (!is_ifpresent && template_selection==SPECIFIC_VALUE) return;
    break;
  case TR_OMIT:
    if (!is_ifpresent && (template_selection==OMIT_VALUE ||
        template_selection==SPECIFIC_VALUE)) return;
    break;
  case TR_PRESENT:
    if (!match_omit(legacy)) return;
    break;
  default:
    return;
  }
  TTCN_error("Restriction `%s' on template of type %s violated.",
             get_res_name(t_res), t_name ? t_name : "EXTERNAL");
}
#endif
