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
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Tatarka, Gabor
 *
 ******************************************************************************/
#ifndef ASN_NULL_HH
#define ASN_NULL_HH

#include "Types.h"
#include "Basetype.hh"
#include "Template.hh"
#include "Optional.hh"
#include "Encdec.hh"
#include "BER.hh"

class Module_Param;

class ASN_NULL : public Base_Type {
  friend boolean operator==(asn_null_type par_value,
    const ASN_NULL& other_value);

  boolean bound_flag;
public:
  ASN_NULL();
  ASN_NULL(asn_null_type other_value);
  ASN_NULL(const ASN_NULL& other_value);

  ASN_NULL& operator=(asn_null_type other_value);
  ASN_NULL& operator=(const ASN_NULL& other_value);

  boolean operator==(asn_null_type other_value) const;
  boolean operator==(const ASN_NULL& other_value) const;
  inline boolean operator!=(asn_null_type other_value) const
    { return !(*this == other_value); }
  inline boolean operator!=(const ASN_NULL& other_value) const
    { return !(*this == other_value); }

  inline boolean is_bound() const { return bound_flag; }
  inline boolean is_value() const { return bound_flag; }
  inline void clean_up() { bound_flag = FALSE; }

  void log() const;

#ifdef TITAN_RUNTIME_2
  boolean is_equal(const Base_Type* other_value) const { return *this == *(static_cast<const ASN_NULL*>(other_value)); }
  void set_value(const Base_Type* other_value) { *this = *(static_cast<const ASN_NULL*>(other_value)); }
  Base_Type* clone() const { return new ASN_NULL(*this); }
  const TTCN_Typedescriptor_t* get_descriptor() const { return &ASN_NULL_descr_; }
  Module_Param* get_param(Module_Param_Name& param_name) const;
#else
  inline boolean is_present() const { return is_bound(); }
#endif

  void set_param(Module_Param& param);

  void encode_text(Text_Buf& text_buf) const;
  void decode_text(Text_Buf& text_buf);

  void encode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf,
              TTCN_EncDec::coding_t p_coding, ...) const;

  void decode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf,
              TTCN_EncDec::coding_t p_coding, ...);

  ASN_BER_TLV_t* BER_encode_TLV(const TTCN_Typedescriptor_t& p_td,
                                unsigned p_coding) const;

  boolean BER_decode_TLV(const TTCN_Typedescriptor_t& p_td,
                         const ASN_BER_TLV_t& p_tlv, unsigned L_form);
  int XER_encode(const XERdescriptor_t& p_td,
                 TTCN_Buffer& p_buf, unsigned int flavor, unsigned int flavor2, int indent, embed_values_enc_struct_t*) const;
  int XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& reader,
                 unsigned int flavor, unsigned int flavor2, embed_values_dec_struct_t*);
  
  /** Encodes accordingly to the JSON encoding rules.
    * Returns the length of the encoded data. */
  int JSON_encode(const TTCN_Typedescriptor_t&, JSON_Tokenizer&) const;
  
  /** Decodes accordingly to the JSON decoding rules.
    * Returns the length of the encoded data. */
  int JSON_decode(const TTCN_Typedescriptor_t&, JSON_Tokenizer&, boolean);
};

extern boolean operator==(asn_null_type par_value, const ASN_NULL& other_value);
inline boolean operator!=(asn_null_type par_value, const ASN_NULL& other_value)
  { return !(par_value == other_value); }

class ASN_NULL_template : public Base_Template {
  struct {
    unsigned int n_values;
    ASN_NULL_template *list_value;
  } value_list;

  void copy_template(const ASN_NULL_template& other_value);

public:
  ASN_NULL_template();
  ASN_NULL_template(template_sel other_value);
  ASN_NULL_template(asn_null_type other_value);
  ASN_NULL_template(const ASN_NULL& other_value);
  ASN_NULL_template(const OPTIONAL<ASN_NULL>& other_value);
  ASN_NULL_template(const ASN_NULL_template& other_value);

  ~ASN_NULL_template();
  void clean_up();

  ASN_NULL_template& operator=(template_sel other_value);
  ASN_NULL_template& operator=(asn_null_type other_value);
  ASN_NULL_template& operator=(const ASN_NULL& other_value);
  ASN_NULL_template& operator=(const OPTIONAL<ASN_NULL>& other_value);
  ASN_NULL_template& operator=(const ASN_NULL_template& other_value);

  boolean match(asn_null_type other_value, boolean legacy = FALSE) const;
  boolean match(const ASN_NULL& other_value, boolean legacy = FALSE) const;
  asn_null_type valueof() const;

  void set_type(template_sel template_type, unsigned int list_length);
  ASN_NULL_template& list_item(unsigned int list_index);

  void log() const;
  void log_match(const ASN_NULL& match_value, boolean legacy = FALSE) const;

  void set_param(Module_Param& param);

  void encode_text(Text_Buf& text_buf) const;
  void decode_text(Text_Buf& text_buf);

  boolean is_present(boolean legacy = FALSE) const;
  boolean match_omit(boolean legacy = FALSE) const;
#ifdef TITAN_RUNTIME_2
  Module_Param* get_param(Module_Param_Name& param_name) const;
  void valueofv(Base_Type* value) const { *(static_cast<ASN_NULL*>(value)) = valueof(); }
  void set_value(template_sel other_value) { *this = other_value; }
  void copy_value(const Base_Type* other_value) { *this = *(static_cast<const ASN_NULL*>(other_value)); }
  Base_Template* clone() const { return new ASN_NULL_template(*this); }
  const TTCN_Typedescriptor_t* get_descriptor() const { return &ASN_NULL_descr_; }
  boolean matchv(const Base_Type* other_value, boolean legacy) const { return match(*(static_cast<const ASN_NULL*>(other_value)), legacy); }
  void log_matchv(const Base_Type* match_value, boolean legacy) const  { log_match(*(static_cast<const ASN_NULL*>(match_value)), legacy); }
#else
  void check_restriction(template_res t_res, const char* t_name=NULL, boolean legacy = FALSE) const;
#endif
};

#endif // ASN_NULL_HH
