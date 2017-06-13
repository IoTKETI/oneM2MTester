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
 *   Tatarka, Gabor
 *
 ******************************************************************************/
#ifndef INTEGER_HH
#define INTEGER_HH

#include "Basetype.hh"
#include "Template.hh"
#include "Error.hh"
#include "RInt.hh"

class BITSTRING;
class CHARSTRING;
class HEXSTRING;
class OCTETSTRING;

class Module_Param;

template<typename T>
class OPTIONAL;

class INTEGER : public Base_Type {
  // Private constructor for internal initialization.  It's not part of the
  // public API.
  explicit INTEGER(BIGNUM *other_value);
  int from_string(const char *);
  
  int get_nof_digits();

  friend class INTEGER_template;
  friend INTEGER operator+(int int_value, const INTEGER& other_value);
  friend INTEGER operator-(int int_value, const INTEGER& other_value);
  friend INTEGER operator*(int int_value, const INTEGER& other_value);
  friend INTEGER operator/(int int_value, const INTEGER& other_value);
  friend INTEGER rem(const INTEGER& left_value, const INTEGER& right_value);
  friend INTEGER rem(const INTEGER& left_value, int right_value);
  friend INTEGER rem(int left_value, const INTEGER& right_value);
  friend INTEGER mod(const INTEGER& left_value, const INTEGER& right_value);
  friend INTEGER mod(const INTEGER& left_value, int right_value);
  friend INTEGER mod(int left_value, const INTEGER& right_value);
  friend boolean operator==(int int_value, const INTEGER& other_value);
  friend boolean operator< (int int_value, const INTEGER& other_value);
  friend boolean operator> (int int_value, const INTEGER& other_value);

  friend INTEGER bit2int(const BITSTRING& value);
  friend INTEGER hex2int(const HEXSTRING& value);
  friend INTEGER oct2int(const OCTETSTRING& value);
  friend INTEGER str2int(const CHARSTRING& value);

  friend void log_param_value();

  boolean bound_flag;
  boolean native_flag;
  union {
    RInt native;
    BIGNUM *openssl;
  } val;

public:
  INTEGER();
  INTEGER(const INTEGER& other_value);
  INTEGER(int other_value);
  explicit INTEGER(const char *other_value);
  ~INTEGER();
  void clean_up();

  INTEGER& operator=(int other_value);
  INTEGER& operator=(const INTEGER& other_value);
  INTEGER& operator++();
  INTEGER& operator--();

  INTEGER operator+() const;
  INTEGER operator-() const;

  INTEGER operator+(int other_value) const;
  INTEGER operator+(const INTEGER& other_value) const;
  INTEGER operator-(int other_value) const;
  INTEGER operator-(const INTEGER& other_value) const;
  INTEGER operator*(int other_value) const;
  INTEGER operator*(const INTEGER& other_value) const;
  INTEGER operator/(int other_value) const;
  INTEGER operator/(const INTEGER& other_value) const;

  boolean operator==(int other_value) const;
  boolean operator==(const INTEGER& other_value) const;
  inline boolean operator!=(int other_value) const
    { return !(*this == other_value); }
  inline boolean operator!=(const INTEGER& other_value) const
    { return !(*this == other_value); }

  boolean operator<(int other_value) const;
  boolean operator<(const INTEGER& other_value) const;
  boolean operator>(int other_value) const;
  boolean operator>(const INTEGER& other_value) const;
  inline boolean operator<=(int other_value) const
    { return !(*this > other_value); }
  inline boolean operator<=(const INTEGER& other_value) const
    { return !(*this > other_value); }
  inline boolean operator>=(int other_value) const
    { return !(*this < other_value); }
  inline boolean operator>=(const INTEGER& other_value) const
    { return !(*this < other_value); }

  operator int() const;
  long long int get_long_long_val() const;
  void set_long_long_val(long long int other_value);

  inline boolean is_native() const { return native_flag; }
  inline boolean is_bound() const { return bound_flag; }
  inline boolean is_value() const { return bound_flag; }
  inline void must_bound(const char *err_msg) const
    { if (!bound_flag)
      TTCN_error("%s", err_msg);
    }
  int_val_t get_val() const;
  void set_val(const int_val_t& other_value);

  void log() const;

#ifdef TITAN_RUNTIME_2
  boolean is_equal(const Base_Type* other_value) const { return *this == *(static_cast<const INTEGER*>(other_value)); }
  void set_value(const Base_Type* other_value) { *this = *(static_cast<const INTEGER*>(other_value)); }
  Base_Type* clone() const { return new INTEGER(*this); }
  const TTCN_Typedescriptor_t* get_descriptor() const { return &INTEGER_descr_; }
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
    * TTCN_Typedescriptor_t.  It must be public because called by
    * another types during encoding.  Returns the length of encoded data.  */
  int RAW_encode(const TTCN_Typedescriptor_t&, RAW_enc_tree&) const;
  int RAW_encode_openssl(const TTCN_Typedescriptor_t&, RAW_enc_tree&) const;

  /** Decodes the value of the variable according to the
    * TTCN_Typedescriptor_t.  It must be public because called by
    * another types during encoding. Returns the number of decoded bits.  */
  int RAW_decode(const TTCN_Typedescriptor_t&, TTCN_Buffer&, int, raw_order_t,
                 boolean no_err=FALSE, int sel_field=-1, boolean first_call=TRUE);
  int TEXT_encode(const TTCN_Typedescriptor_t&, TTCN_Buffer&) const;
  int TEXT_decode(const TTCN_Typedescriptor_t&, TTCN_Buffer&,
                  Limit_Token_List&, boolean no_err = FALSE, boolean first_call=TRUE);
  /** @brief Encode according to XML Encoding Rules.
   **/
  int XER_encode(const XERdescriptor_t& p_td, TTCN_Buffer& p_buf, unsigned int flavor,
                 unsigned int flavor2, int indent, embed_values_enc_struct_t*) const;
  /** @brief Decode according to XML Encoding Rules.
   **/
  int XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& reader,
                 unsigned int flavor, unsigned int flavor2, embed_values_dec_struct_t*);
  
  /** Encodes accordingly to the JSON encoding rules.
    * Returns the length of the encoded data. */
  int JSON_encode(const TTCN_Typedescriptor_t&, JSON_Tokenizer&) const;
  
  /** Decodes accordingly to the JSON encoding rules.
    * Returns the length of the decoded data. */
  int JSON_decode(const TTCN_Typedescriptor_t&, JSON_Tokenizer&, boolean);
};

extern INTEGER operator+(int int_value, const INTEGER& other_value);
extern INTEGER operator-(int int_value, const INTEGER& other_value);
extern INTEGER operator*(int int_value, const INTEGER& other_value);
extern INTEGER operator/(int int_value, const INTEGER& other_value);

extern INTEGER rem(int left_value, int right_value);
extern INTEGER rem(const INTEGER& left_value, const INTEGER& right_value);
extern INTEGER rem(const INTEGER& left_value, int right_value);
extern INTEGER rem(int left_value, const INTEGER& right_value);

extern INTEGER mod(int left_value, int right_value);
extern INTEGER mod(const INTEGER& left_value, const INTEGER& right_value);
extern INTEGER mod(const INTEGER& left_value, int right_value);
extern INTEGER mod(int left_value, const INTEGER& right_value);

extern boolean operator==(int int_value, const INTEGER& other_value);
extern boolean operator<(int int_value, const INTEGER& other_value);
extern boolean operator>(int int_value, const INTEGER& other_value);

inline boolean operator!=(int int_value, const INTEGER& other_value)
{
  return !(int_value == other_value);
}

inline boolean operator<=(int int_value, const INTEGER& other_value)
{
  return !(int_value > other_value);
}

inline boolean operator>=(int int_value, const INTEGER& other_value)
{
  return !(int_value < other_value);
}

// Integer template class.

class INTEGER_template : public Base_Template {
private:
  union {
    struct {
      boolean native_flag;
      union {
        RInt native;
        BIGNUM *openssl;
      } val;
    } int_val;
    struct {
      unsigned int n_values;
      INTEGER_template *list_value;
    } value_list;
    struct {
      boolean min_is_present, max_is_present;
      boolean min_is_exclusive, max_is_exclusive;
      struct {
        boolean native_flag;
        union {
          RInt native;
          BIGNUM *openssl;
        } val;
      } min_value, max_value;
    } value_range;
  };

  void copy_template(const INTEGER_template& other_value);

public:
  INTEGER_template();
  INTEGER_template(const INTEGER_template& other_value);
  INTEGER_template(template_sel other_value);
  INTEGER_template(int other_value);
  INTEGER_template(const INTEGER& other_value);
  INTEGER_template(const OPTIONAL<INTEGER>& other_value);
  ~INTEGER_template();
  void clean_up();

  INTEGER_template& operator=(template_sel other_value);
  INTEGER_template& operator=(int other_value);
  INTEGER_template& operator=(const INTEGER& other_value);
  INTEGER_template& operator=(const OPTIONAL<INTEGER>& other_value);
  INTEGER_template& operator=(const INTEGER_template& other_value);

  boolean match(int other_value, boolean legacy = FALSE) const;
  boolean match(const INTEGER& other_value, boolean legacy = FALSE) const;
  INTEGER valueof() const;

  /** Sets the template type.
   *
   * Calls clean_up(), so the template becomes uninitialized.
   *
   * @param template_type
   * @param list_length used for VALUE_LIST and COMPLEMENTED_LIST only.
   *
   * @post template_selection = UNINITIALIZED_TEMPLATE
   */
  void set_type(template_sel template_type, unsigned int list_length = 0);
  /** Returns the specified list item
   *
   * @param list_index
   * @return the list item
   * @pre template_selection is VALUE_LIST or COMPLEMENTED_LIST
   */
  INTEGER_template& list_item(unsigned int list_index);

  /** Sets the lower bound of the value range.
   *
   * @param min_value
   * @pre template_selection == VALUE_RANGE; else DTE
   */
  void set_min(int min_value);
  /** Sets the lower bound of the value range.
   *
   * @param min_value
   * @pre template_selection == VALUE_RANGE; else DTE
   * @pre min_value must be bound; else DTE
   */
  void set_min(const INTEGER& min_value);
  /** Sets the upper bound of the value range.
   *
   * @param max_value
   * @pre template_selection == VALUE_RANGE; else DTE
   * @pre min_value must be bound; else DTE
   */
  void set_max(int max_value);
  /** Sets the upper bound of the value range.
   *
   * @param max_value
   * @pre template_selection == VALUE_RANGE; else DTE
   * @pre min_value must be bound; else DTE
   */
  void set_max(const INTEGER& max_value);
  
  void set_min_exclusive(boolean min_exclusive);
  void set_max_exclusive(boolean max_exclusive);

  void log() const;
  void log_match(const INTEGER& match_value, boolean legacy = FALSE) const;

  void set_param(Module_Param& param);

  void encode_text(Text_Buf& text_buf) const;
  void decode_text(Text_Buf& text_buf);

  boolean is_present(boolean legacy = FALSE) const;
  boolean match_omit(boolean legacy = FALSE) const;

#ifdef TITAN_RUNTIME_2
  Module_Param* get_param(Module_Param_Name& param_name) const;
  void valueofv(Base_Type* value) const { *(static_cast<INTEGER*>(value)) = valueof(); }
  void set_value(template_sel other_value) { *this = other_value; }
  void copy_value(const Base_Type* other_value) { *this = *(static_cast<const INTEGER*>(other_value)); }
  Base_Template* clone() const { return new INTEGER_template(*this); }
  const TTCN_Typedescriptor_t* get_descriptor() const { return &INTEGER_descr_; }
  boolean matchv(const Base_Type* other_value, boolean legacy) const { return match(*(static_cast<const INTEGER*>(other_value)), legacy); }
  void log_matchv(const Base_Type* match_value, boolean legacy) const  { log_match(*(static_cast<const INTEGER*>(match_value)), legacy); }
#else
  void check_restriction(template_res t_res, const char* t_name=NULL, boolean legacy = FALSE) const;
#endif
};

#endif  // INTEGER_HH

