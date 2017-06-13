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
#ifndef OBJID_HH
#define OBJID_HH

#include "Basetype.hh"
#include "Template.hh"

template<typename T>
class OPTIONAL;

class Text_Buf;
class Module_Param;
class INTEGER;

/** Runtime class for object identifiers (objid)
 *
 */
class OBJID : public Base_Type {
  /** No user serviceable parts. */
  struct objid_struct;
  objid_struct *val_ptr;

  void init_struct(int n_components);
  void copy_value();
  
  /** Initializes the object identifier with a string containing the components
    * separated by dots. */
  void from_string(char* p_str);

public:
  typedef unsigned int objid_element;
  OBJID();
  OBJID(int init_n_components, ...);
  OBJID(int init_n_components, const objid_element *init_components);
  OBJID(const OBJID& other_value);

  ~OBJID();

  OBJID& operator=(const OBJID& other_value);

  boolean operator==(const OBJID& other_value) const;
  inline boolean operator!=(const OBJID& other_value) const
    { return !(*this == other_value); }

  objid_element& operator[](int index_value);
  objid_element  operator[](int index_value) const;

  inline boolean is_bound() const { return val_ptr != NULL; }
  inline boolean is_value() const { return val_ptr != NULL; }
  void clean_up();

  int size_of() const;
  int lengthof() const { return size_of(); } // for backward compatibility

  operator const objid_element*() const;
  
  static objid_element from_INTEGER(const INTEGER& p_int);

#ifdef TITAN_RUNTIME_2
  boolean is_equal(const Base_Type* other_value) const { return *this == *(static_cast<const OBJID*>(other_value)); }
  void set_value(const Base_Type* other_value) { *this = *(static_cast<const OBJID*>(other_value)); }
  Base_Type* clone() const { return new OBJID(*this); }
  const TTCN_Typedescriptor_t* get_descriptor() const { return &OBJID_descr_; }
  Module_Param* get_param(Module_Param_Name& param_name) const;
#else
  inline boolean is_present() const { return is_bound(); }
#endif

  void log() const;
  
  void set_param(Module_Param& param);
  
  void encode_text(Text_Buf& text_buf) const;
  void decode_text(Text_Buf& text_buf);

  void encode(const TTCN_Typedescriptor_t&, TTCN_Buffer&,
              TTCN_EncDec::coding_t, ...) const;

  void decode(const TTCN_Typedescriptor_t&, TTCN_Buffer&,
              TTCN_EncDec::coding_t, ...);

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

// objid template class

class OBJID_template : public Base_Template {
  OBJID single_value;
  struct {
    unsigned int n_values;
    OBJID_template *list_value;
  } value_list;

  void copy_template(const OBJID_template& other_value);

public:
  OBJID_template();
  OBJID_template(template_sel other_value);
  OBJID_template(const OBJID& other_value);
  OBJID_template(const OPTIONAL<OBJID>& other_value);
  OBJID_template(const OBJID_template& other_value);

  ~OBJID_template();
  void clean_up();

  OBJID_template& operator=(template_sel other_value);
  OBJID_template& operator=(const OBJID& other_value);
  OBJID_template& operator=(const OPTIONAL<OBJID>& other_value);
  OBJID_template& operator=(const OBJID_template& other_value);

  boolean match(const OBJID& other_value, boolean legacy = FALSE) const;
  const OBJID& valueof() const;

  int size_of() const;

  void set_type(template_sel template_type, unsigned int list_length);
  OBJID_template& list_item(unsigned int list_index);

  void log() const;
  void log_match(const OBJID& match_value, boolean legacy = FALSE) const;

  void set_param(Module_Param& param);

  void encode_text(Text_Buf& text_buf) const;
  void decode_text(Text_Buf& text_buf);

  boolean is_present(boolean legacy = FALSE) const;
  boolean match_omit(boolean legacy = FALSE) const;
#ifdef TITAN_RUNTIME_2
  Module_Param* get_param(Module_Param_Name& param_name) const;
  void valueofv(Base_Type* value) const { *(static_cast<OBJID*>(value)) = valueof(); }
  void set_value(template_sel other_value) { *this = other_value; }
  void copy_value(const Base_Type* other_value) { *this = *(static_cast<const OBJID*>(other_value)); }
  Base_Template* clone() const { return new OBJID_template(*this); }
  const TTCN_Typedescriptor_t* get_descriptor() const { return &OBJID_descr_; }
  boolean matchv(const Base_Type* other_value, boolean legacy) const { return match(*(static_cast<const OBJID*>(other_value)), legacy); }
  void log_matchv(const Base_Type* match_value, boolean legacy) const  { log_match(*(static_cast<const OBJID*>(match_value)), legacy); }
#else
  void check_restriction(template_res t_res, const char* t_name=NULL, boolean legacy = FALSE) const;
#endif
};

typedef OBJID ASN_ROID;
typedef OBJID_template ASN_ROID_template;

#endif
