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
 *   Szalai, Gabor
 *   Tatarka, Gabor
 *
 ******************************************************************************/
#ifndef BOOLEAN_HH
#define BOOLEAN_HH

#include "Types.h"
#include "Basetype.hh"
#include "Template.hh"
#include "Error.hh"

class Module_Param;

template<typename T>
class OPTIONAL;

class BOOLEAN : public Base_Type {
  friend class BOOLEAN_template;

  friend boolean operator&&(boolean bool_value, const BOOLEAN& other_value);
  friend boolean operator^(boolean bool_value, const BOOLEAN& other_value);
  friend boolean operator||(boolean bool_value, const BOOLEAN& other_value);
  friend boolean operator==(boolean bool_value, const BOOLEAN& other_value);

  boolean bound_flag;
  boolean boolean_value;

public:
  BOOLEAN();
  BOOLEAN(boolean other_value);
  BOOLEAN(const BOOLEAN& other_value);

  BOOLEAN& operator=(boolean other_value);
  BOOLEAN& operator=(const BOOLEAN& other_value);

  boolean operator!() const;
  boolean operator&&(boolean other_value) const;
  boolean operator&&(const BOOLEAN& other_value) const;
  boolean operator^(boolean other_value) const;
  boolean operator^(const BOOLEAN& other_value) const;
  boolean operator||(const BOOLEAN& other_value) const;
  boolean operator||(boolean other_value) const;

  boolean operator==(boolean other_value) const;
  boolean operator==(const BOOLEAN& other_value) const;

  inline boolean operator!=(boolean other_value) const
    { return !(*this == other_value); }
  inline boolean operator!=(const BOOLEAN& other_value) const
    { return !(*this == other_value); }

  operator boolean() const;

  inline boolean is_bound() const { return bound_flag; }
  inline boolean is_value() const { return bound_flag; }
  void clean_up();
  inline void must_bound(const char *err_msg) const
    { if (!bound_flag) TTCN_error("%s", err_msg); }

#ifdef TITAN_RUNTIME_2
  boolean is_equal(const Base_Type* other_value) const { return *this == *(static_cast<const BOOLEAN*>(other_value)); }
  void set_value(const Base_Type* other_value) { *this = *(static_cast<const BOOLEAN*>(other_value)); }
  Base_Type* clone() const { return new BOOLEAN(*this); }
  const TTCN_Typedescriptor_t* get_descriptor() const { return &BOOLEAN_descr_; }
  Module_Param* get_param(Module_Param_Name& param_name) const;
#else
  inline boolean is_present() const { return is_bound(); }
#endif

  void log() const;

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

  /** Encodes the value of the variable according to the
    * TTCN_Typedescriptor_t. It must be public because called by
    * another types during encoding. Returns the length of encoded data*/
  int RAW_encode(const TTCN_Typedescriptor_t&, RAW_enc_tree&) const;

  /** Decodes the value of the variable according to the
    * TTCN_Typedescriptor_t. It must be public because called by
    * another types during encoding. Returns the number of decoded bits*/
  int RAW_decode(const TTCN_Typedescriptor_t&, TTCN_Buffer&, int, raw_order_t,
                 boolean no_err=FALSE, int sel_field=-1, boolean first_call=TRUE);
  int TEXT_encode(const TTCN_Typedescriptor_t&,
                 TTCN_Buffer&) const;
  int TEXT_decode(const TTCN_Typedescriptor_t&, TTCN_Buffer&,  Limit_Token_List&,
                  boolean no_err=FALSE, boolean first_call=TRUE);
  int XER_encode(const XERdescriptor_t& p_td,
                 TTCN_Buffer& p_buf, unsigned int flavor, unsigned int flavor2, int indent, embed_values_enc_struct_t*) const;
  int XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& reader,
                 unsigned int flavor, unsigned int flavor2, embed_values_dec_struct_t*);
  
  /** Encodes accordingly to the JSON encoding rules.
    * Returns the length of the encoded data. */
  int JSON_encode(const TTCN_Typedescriptor_t&, JSON_Tokenizer&) const;
  
  /** Decodes accordingly to the JSON encoding rules.
    * Returns the length of the decoded data. */
  int JSON_decode(const TTCN_Typedescriptor_t&, JSON_Tokenizer&, boolean);
};

extern boolean operator&&(boolean bool_value, const BOOLEAN& other_value);
extern boolean operator^(boolean bool_value, const BOOLEAN& other_value);
extern boolean operator||(boolean bool_value, const BOOLEAN& other_value);

extern boolean operator==(boolean bool_value, const BOOLEAN& other_value);
inline boolean operator!=(boolean bool_value, const BOOLEAN& other_value)
  { return !(bool_value == other_value); }


// boolean template class


class BOOLEAN_template : public Base_Template {
  union {
    boolean single_value;
    struct {
      unsigned int n_values;
      BOOLEAN_template *list_value;
    } value_list;
  };

  void copy_template(const BOOLEAN_template& other_value);

public:

  BOOLEAN_template();
  BOOLEAN_template(template_sel other_value);
  BOOLEAN_template(boolean other_value);
  BOOLEAN_template(const BOOLEAN& other_value);
  BOOLEAN_template(const OPTIONAL<BOOLEAN>& other_value);
  BOOLEAN_template(const BOOLEAN_template& other_value);

  ~BOOLEAN_template();
  void clean_up();

  BOOLEAN_template& operator=(template_sel other_value);
  BOOLEAN_template& operator=(boolean other_value);
  BOOLEAN_template& operator=(const BOOLEAN& other_value);
  BOOLEAN_template& operator=(const OPTIONAL<BOOLEAN>& other_value);
  BOOLEAN_template& operator=(const BOOLEAN_template& other_value);

  boolean match(boolean other_value, boolean legacy = FALSE) const;
  boolean match(const BOOLEAN& other_value, boolean legacy = FALSE) const;
  boolean valueof() const;

  void set_type(template_sel template_type, unsigned int list_length);
  BOOLEAN_template& list_item(unsigned int list_index);

  void log() const;
  void log_match(const BOOLEAN& match_value, boolean legacy = FALSE) const;

  void set_param(Module_Param& param);

  void encode_text(Text_Buf& text_buf) const;
  void decode_text(Text_Buf& text_buf);

  boolean is_present(boolean legacy = FALSE) const;
  boolean match_omit(boolean legacy = FALSE) const;
#ifdef TITAN_RUNTIME_2
  Module_Param* get_param(Module_Param_Name& param_name) const;
  void valueofv(Base_Type* value) const { *(static_cast<BOOLEAN*>(value)) = valueof(); }
  void set_value(template_sel other_value) { *this = other_value; }
  void copy_value(const Base_Type* other_value) { *this = *(static_cast<const BOOLEAN*>(other_value)); }
  Base_Template* clone() const { return new BOOLEAN_template(*this); }
  const TTCN_Typedescriptor_t* get_descriptor() const { return &BOOLEAN_descr_; }
  boolean matchv(const Base_Type* other_value, boolean legacy) const { return match(*(static_cast<const BOOLEAN*>(other_value)), legacy); }
  void log_matchv(const Base_Type* match_value, boolean legacy) const  { log_match(*(static_cast<const BOOLEAN*>(match_value)), legacy); }
#else
  void check_restriction(template_res t_res, const char* t_name=NULL, boolean legacy = FALSE) const;
#endif
};

#endif
