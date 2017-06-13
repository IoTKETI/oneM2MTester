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
#ifndef VERDICTTYPE_HH
#define VERDICTTYPE_HH

#include "Basetype.hh"
#include "Template.hh"

class Module_Param;

template<typename T>
class OPTIONAL;

/// verdicttype value class

class VERDICTTYPE: public Base_Type {
  friend class VERDICTTYPE_template;
  friend boolean operator==(verdicttype par_value,
    const VERDICTTYPE& other_value);

  verdicttype verdict_value;
  verdicttype str_to_verdict(const char *v, boolean silent = FALSE);
public:
  /** Default constructor.
   * Initialises \p verdict_value to UNBOUND_VERDICT, a private macro
   * outside the range of \p verdicttype enum.
   *
   * @post \p is_bound() would return \p false */
  VERDICTTYPE();
  VERDICTTYPE(verdicttype other_value);
  VERDICTTYPE(const VERDICTTYPE& other_value);

  VERDICTTYPE& operator=(verdicttype other_value);
  VERDICTTYPE& operator=(const VERDICTTYPE& other_value);

  boolean operator==(verdicttype other_value) const;
  boolean operator==(const VERDICTTYPE& other_value) const;
  inline boolean operator!=(verdicttype other_value) const
  {
    return !(*this == other_value);
  }
  inline boolean operator!=(const VERDICTTYPE& other_value) const
  {
    return !(*this == other_value);
  }

  operator verdicttype() const;

  inline boolean is_bound() const
  {
    return verdict_value >= NONE && verdict_value <= ERROR;
  }
  inline boolean is_value() const
  {
    return is_bound();
  }
  void clean_up();

#ifdef TITAN_RUNTIME_2
  boolean is_equal(const Base_Type* other_value) const {return *this == *(static_cast<const VERDICTTYPE*>(other_value));}
  void set_value(const Base_Type* other_value) {*this = *(static_cast<const VERDICTTYPE*>(other_value));}
  Base_Type* clone() const {return new VERDICTTYPE(*this);}
  const TTCN_Typedescriptor_t* get_descriptor() const {return &VERDICTTYPE_descr_;}
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

  int XER_encode(const XERdescriptor_t& p_td, TTCN_Buffer& p_buf,
    unsigned int flavor, unsigned int flavor2, int indent, embed_values_enc_struct_t*) const;
  int XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& reader,
    unsigned int flavor, unsigned int flavor2, embed_values_dec_struct_t*);
  
  /** Encodes accordingly to the JSON encoding rules.
    * Returns the length of the encoded data. */
  int JSON_encode(const TTCN_Typedescriptor_t&, JSON_Tokenizer&) const;
  
  /** Decodes accordingly to the JSON encoding rules.
    * Returns the length of the decoded data. */
  int JSON_decode(const TTCN_Typedescriptor_t&, JSON_Tokenizer&, boolean);
};

inline boolean operator==(verdicttype par_value, verdicttype other_value)
{
  return (int) par_value == (int) other_value;
}

inline boolean operator!=(verdicttype par_value, verdicttype other_value)
{
  return (int) par_value != (int) other_value;
}

extern boolean operator==(verdicttype par_value, const VERDICTTYPE& other_value);

inline boolean operator!=(verdicttype par_value, const VERDICTTYPE& other_value)
{
  return !(par_value == other_value);
}

// verdicttype template class

class VERDICTTYPE_template: public Base_Template {
  union {
    verdicttype single_value;
    struct {
      unsigned int n_values;
      VERDICTTYPE_template *list_value;
    } value_list;
  };

  void copy_value(const VERDICTTYPE& other_value);
  void copy_template(const VERDICTTYPE_template& other_value);

public:
  VERDICTTYPE_template();
  VERDICTTYPE_template(template_sel other_value);
  VERDICTTYPE_template(verdicttype other_value);
  VERDICTTYPE_template(const VERDICTTYPE& other_value);
  VERDICTTYPE_template(const OPTIONAL<VERDICTTYPE>& other_value);
  VERDICTTYPE_template(const VERDICTTYPE_template& other_value);

  ~VERDICTTYPE_template();
  void clean_up();

  VERDICTTYPE_template& operator=(template_sel other_value);
  VERDICTTYPE_template& operator=(verdicttype other_value);
  VERDICTTYPE_template& operator=(const VERDICTTYPE& other_value);
  VERDICTTYPE_template& operator=(const OPTIONAL<VERDICTTYPE>& other_value);
  VERDICTTYPE_template& operator=(const VERDICTTYPE_template& other_value);

  boolean match(verdicttype other_value, boolean legacy = FALSE) const;
  boolean match(const VERDICTTYPE& other_value, boolean legacy = FALSE) const;
  verdicttype valueof() const;

  void set_type(template_sel template_type, unsigned int list_length);
  VERDICTTYPE_template& list_item(unsigned int list_index);

  void log() const;
  void log_match(const VERDICTTYPE& match_value, boolean legacy = FALSE) const;

  void set_param(Module_Param& param);

  void encode_text(Text_Buf& text_buf) const;
  void decode_text(Text_Buf& text_buf);

  boolean is_present(boolean legacy = FALSE) const;
  boolean match_omit(boolean legacy = FALSE) const;
#ifdef TITAN_RUNTIME_2
  Module_Param* get_param(Module_Param_Name& param_name) const;
  void valueofv(Base_Type* value) const {*(static_cast<VERDICTTYPE*>(value)) = valueof();}
  void set_value(template_sel other_value) {*this = other_value;}
  void copy_value(const Base_Type* other_value) {*this = *(static_cast<const VERDICTTYPE*>(other_value));}
  Base_Template* clone() const {return new VERDICTTYPE_template(*this);}
  const TTCN_Typedescriptor_t* get_descriptor() const {return &VERDICTTYPE_descr_;}
  boolean matchv(const Base_Type* other_value, boolean legacy) const {return match(*(static_cast<const VERDICTTYPE*>(other_value)), legacy);}
  void log_matchv(const Base_Type* match_value, boolean legacy) const {log_match(*(static_cast<const VERDICTTYPE*>(match_value)), legacy);}
#else
  void check_restriction(template_res t_res, const char* t_name=NULL, boolean legacy = FALSE) const;
#endif
};

#endif
