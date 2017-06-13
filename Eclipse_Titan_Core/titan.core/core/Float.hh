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
#ifndef FLOAT_HH
#define FLOAT_HH

#include "Basetype.hh"
#include "Template.hh"
#include "Error.hh"
#include "ttcn3float.hh"

class Module_Param;

template<typename T>
class OPTIONAL;

// float value class

class FLOAT : public Base_Type {

  friend class FLOAT_template;
  friend class TIMER;

  friend double operator+(double double_value, const FLOAT& other_value);
  friend double operator-(double double_value, const FLOAT& other_value);
  friend double operator*(double double_value, const FLOAT& other_value);
  friend double operator/(double double_value, const FLOAT& other_value);

  friend boolean operator==(double double_value, const FLOAT& other_value);
  friend boolean operator< (double double_value, const FLOAT& other_value);
  friend boolean operator> (double double_value, const FLOAT& other_value);

  boolean bound_flag;
  ttcn3float float_value;
  
  /** Returns true if the string parameter contains the string representation 
    * of a real number, otherwise returns false. */
  boolean is_float(const char* p_str);

public:

  FLOAT();
  FLOAT(double other_value);
  FLOAT(const FLOAT& other_value);
  void clean_up();

  FLOAT& operator=(double other_value);
  FLOAT& operator=(const FLOAT& other_value);

  double operator+() const;
  double operator-() const;

  double operator+(double other_value) const;
  double operator+(const FLOAT& other_value) const;
  double operator-(double other_value) const;
  double operator-(const FLOAT& other_value) const;
  double operator*(double other_value) const;
  double operator*(const FLOAT& other_value) const;
  double operator/(double other_value) const;
  double operator/(const FLOAT& other_value) const;

  boolean operator==(double other_value) const;
  boolean operator==(const FLOAT& other_value) const;
  inline boolean operator!=(double other_value) const
  { return !(*this == other_value); }
  inline boolean operator!=(const FLOAT& other_value) const
  { return !(*this == other_value); }

  boolean operator<(double other_value) const;
  boolean operator<(const FLOAT& other_value) const;
  boolean operator>(double other_value) const;
  boolean operator>(const FLOAT& other_value) const;
  inline boolean operator<=(double other_value) const
  { return !(*this > other_value); }
  inline boolean operator<=(const FLOAT& other_value) const
  { return !(*this > other_value); }
  inline boolean operator>=(double other_value) const
  { return !(*this < other_value); }
  inline boolean operator>=(const FLOAT& other_value) const
  { return !(*this < other_value); }

  operator double() const;

  inline boolean is_bound() const { return bound_flag; }
  inline boolean is_value() const { return bound_flag; }
  inline void must_bound(const char *err_msg) const
    { if (!bound_flag) TTCN_error("%s", err_msg); }
  
  /** special TTCN-3 float values are not_a_number and +/- infinity */
  static boolean is_special(double flt_val);
  static void check_numeric(double flt_val, const char *err_msg_begin);

  void log() const;

#ifdef TITAN_RUNTIME_2
  boolean is_equal(const Base_Type* other_value) const { return *this == *(static_cast<const FLOAT*>(other_value)); }
  void set_value(const Base_Type* other_value) { *this = *(static_cast<const FLOAT*>(other_value)); }
  Base_Type* clone() const { return new FLOAT(*this); }
  const TTCN_Typedescriptor_t* get_descriptor() const { return &FLOAT_descr_; }
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

  /** Encodes the value of the variable according to the
    * TTCN_Typedescriptor_t. It must be public because called by
    * another types during encoding. Returns the length of encoded data*/
  int RAW_encode(const TTCN_Typedescriptor_t&, RAW_enc_tree&) const;

  /** Decodes the value of the variable according to the
    * TTCN_Typedescriptor_t. It must be public because called by
    * another types during encoding. Returns the number of decoded bits*/
  int RAW_decode(const TTCN_Typedescriptor_t&,
                 TTCN_Buffer&, int, raw_order_t, boolean no_err=FALSE,
                 int sel_field=-1, boolean first_call=TRUE);

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

extern double operator+(double double_value, const FLOAT& other_value);
extern double operator-(double double_value, const FLOAT& other_value);
extern double operator*(double double_value, const FLOAT& other_value);
extern double operator/(double double_value, const FLOAT& other_value);

extern boolean operator==(double double_value, const FLOAT& other_value);
extern boolean operator<(double double_value, const FLOAT& other_value);
extern boolean operator>(double double_value, const FLOAT& other_value);

inline boolean operator!=(double double_value, const FLOAT& other_value)
{
  return !(double_value == other_value);
}

inline boolean operator<=(double double_value, const FLOAT& other_value)
{
  return !(double_value > other_value);
}

inline boolean operator>=(double double_value, const FLOAT& other_value)
{
  return !(double_value < other_value);
}

// float template class

class FLOAT_template : public Base_Template {
private:
  union {
    double single_value;
    struct {
      unsigned int n_values;
      FLOAT_template *list_value;
    } value_list;
    struct {
      double min_value, max_value;
      boolean min_is_present, max_is_present;
      boolean min_is_exclusive, max_is_exclusive;
    } value_range;
  };

  void copy_template(const FLOAT_template& other_value);

public:
  FLOAT_template();
  FLOAT_template(template_sel other_value);
  FLOAT_template(double other_value);
  FLOAT_template(const FLOAT& other_value);
  FLOAT_template(const OPTIONAL<FLOAT>& other_value);
  FLOAT_template(const FLOAT_template& other_value);

  ~FLOAT_template();
  void clean_up();

  FLOAT_template& operator=(template_sel other_value);
  FLOAT_template& operator=(double other_value);
  FLOAT_template& operator=(const FLOAT& other_value);
  FLOAT_template& operator=(const OPTIONAL<FLOAT>& other_value);
  FLOAT_template& operator=(const FLOAT_template& other_value);

  boolean match(double other_value, boolean legacy = FALSE) const;
  boolean match(const FLOAT& other_value, boolean legacy = FALSE) const;

  void set_type(template_sel template_type, unsigned int list_length = 0);
  FLOAT_template& list_item(unsigned int list_index);

  void set_min(double min_value);
  void set_min(const FLOAT& min_value);
  void set_max(double max_value);
  void set_max(const FLOAT& max_value);
  void set_min_exclusive(boolean min_exclusive);
  void set_max_exclusive(boolean max_exclusive);

  double valueof() const;

  void log() const;
  void log_match(const FLOAT& match_value, boolean legacy = FALSE) const;

  void set_param(Module_Param& param);

  void encode_text(Text_Buf& text_buf) const;
  void decode_text(Text_Buf& text_buf);

  boolean is_present(boolean legacy = FALSE) const;
  boolean match_omit(boolean legacy = FALSE) const;
#ifdef TITAN_RUNTIME_2
  Module_Param* get_param(Module_Param_Name& param_name) const;
  void valueofv(Base_Type* value) const { *(static_cast<FLOAT*>(value)) = valueof(); }
  void set_value(template_sel other_value) { *this = other_value; }
  void copy_value(const Base_Type* other_value) { *this = *(static_cast<const FLOAT*>(other_value)); }
  Base_Template* clone() const { return new FLOAT_template(*this); }
  const TTCN_Typedescriptor_t* get_descriptor() const { return &FLOAT_descr_; }
  boolean matchv(const Base_Type* other_value, boolean legacy) const { return match(*(static_cast<const FLOAT*>(other_value)), legacy); }
  void log_matchv(const Base_Type* match_value, boolean legacy) const  { log_match(*(static_cast<const FLOAT*>(match_value)), legacy); }
#else
  void check_restriction(template_res t_res, const char* t_name=NULL, boolean legacy = FALSE) const;
#endif
};

extern const FLOAT PLUS_INFINITY, MINUS_INFINITY, NOT_A_NUMBER;

#endif
