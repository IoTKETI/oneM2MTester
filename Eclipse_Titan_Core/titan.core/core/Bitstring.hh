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
 *   Tatarka, Gabor
 *
 ******************************************************************************/
#ifndef BITSTRING_HH
#define BITSTRING_HH

#include "Types.h"
#include "Basetype.hh"
#include "Template.hh"
#include "RAW.hh"
#include "BER.hh"
#include "Error.hh"
#ifdef TITAN_RUNTIME_2
#include "Vector.hh"
#endif

class INTEGER;
class HEXSTRING;
class OCTETSTRING;
class CHARSTRING;
class BITSTRING_ELEMENT;

class Module_Param;

template<typename T>
class OPTIONAL;

/** bitstring value class.
 * Refcounted copy-on-write implementation */
class BITSTRING : public Base_Type {

  friend class BITSTRING_ELEMENT;
  friend class BITSTRING_template;

  friend BITSTRING int2bit(const INTEGER& value, int length);
  friend BITSTRING hex2bit(const HEXSTRING& value);
  friend BITSTRING oct2bit(const OCTETSTRING& value);
  friend BITSTRING str2bit(const CHARSTRING& value);
  friend BITSTRING substr(const BITSTRING& value, int index, int returncount);
  friend BITSTRING replace(const BITSTRING& value, int index, int len,
		                   const BITSTRING& repl);

  struct bitstring_struct;
  bitstring_struct *val_ptr;

  /** Allocate memory if needed.
   * @param n_bits the number of bits needed.
   * @pre   n_bits  >= 0
   * @post  val_ptr != 0
   * If n_bits > 0, allocates n_bits/8 bytes of memory
   * */
  void init_struct(int n_bits);
  /// Get the bit at the given index.
  boolean get_bit(int bit_index) const;
  /// Assign \p new_value to the bit at the given index
  void set_bit(int bit_index, boolean new_value);
  /// Copy-on-write
  void copy_value();
  /** Ensures that if the bitstring length is not a multiple of 8 then
   *  the unused bits in the last byte are all cleared. */
  void clear_unused_bits() const;
  /// Creates a BITSTRING with pre-allocated but uninitialised memory.
  BITSTRING(int n_bits);

public:
  BITSTRING();
  BITSTRING(int init_n_bits, const unsigned char* init_bits);

  /** Copy constructor.
   *
   * @pre \p other_value must be bound */
  BITSTRING(const BITSTRING& other_value);

  /** Creates a BITSTRING with a single bit.
   * @pre \p other_value must be bound */
  BITSTRING(const BITSTRING_ELEMENT& other_value);

  ~BITSTRING();
  /// Decrement the reference count and free the memory if it's the last reference
  void clean_up();

  BITSTRING& operator=(const BITSTRING& other_value);
  BITSTRING& operator=(const BITSTRING_ELEMENT& other_value);

  boolean operator==(const BITSTRING& other_value) const;
  boolean operator==(const BITSTRING_ELEMENT& other_value) const;

  inline boolean operator!=(const BITSTRING& other_value) const
    { return !(*this == other_value); }
  inline boolean operator!=(const BITSTRING_ELEMENT& other_value) const
    { return !(*this == other_value); }

  BITSTRING operator+(const BITSTRING& other_value) const;
  BITSTRING operator+(const BITSTRING_ELEMENT& other_value) const;
#ifdef TITAN_RUNTIME_2
  BITSTRING operator+(const OPTIONAL<BITSTRING>& other_value) const;
#endif

  BITSTRING operator~() const;
  BITSTRING operator&(const BITSTRING& other_value) const;
  BITSTRING operator&(const BITSTRING_ELEMENT& other_value) const;
  BITSTRING operator|(const BITSTRING& other_value) const;
  BITSTRING operator|(const BITSTRING_ELEMENT& other_value) const;
  BITSTRING operator^(const BITSTRING& other_value) const;
  BITSTRING operator^(const BITSTRING_ELEMENT& other_value) const;

  BITSTRING operator<<(int shift_count) const;
  BITSTRING operator<<(const INTEGER& shift_count) const;
  BITSTRING operator>>(int shift_count) const;
  BITSTRING operator>>(const INTEGER& shift_count) const;
  BITSTRING operator<<=(int rotate_count) const;
  BITSTRING operator<<=(const INTEGER& rotate_count) const;
  BITSTRING operator>>=(int rotate_count) const;
  BITSTRING operator>>=(const INTEGER& rotate_count) const;

  BITSTRING_ELEMENT operator[](int index_value);
  BITSTRING_ELEMENT operator[](const INTEGER& index_value);
  const BITSTRING_ELEMENT operator[](int index_value) const;
  const BITSTRING_ELEMENT operator[](const INTEGER& index_value) const;

  inline boolean is_bound() const { return val_ptr != NULL; }
  inline boolean is_value() const { return val_ptr != NULL; }
  inline void must_bound(const char *err_msg) const
    { if (val_ptr == NULL) TTCN_error("%s", err_msg); }

  int lengthof() const;

  operator const unsigned char*() const;

  void log() const;

#ifdef TITAN_RUNTIME_2
  boolean is_equal(const Base_Type* other_value) const { return *this == *(static_cast<const BITSTRING*>(other_value)); }
  void set_value(const Base_Type* other_value) { *this = *(static_cast<const BITSTRING*>(other_value)); }
  Base_Type* clone() const { return new BITSTRING(*this); }
  const TTCN_Typedescriptor_t* get_descriptor() const { return &BITSTRING_descr_; }
  virtual int RAW_encode_negtest_raw(RAW_enc_tree& p_myleaf) const;
  Module_Param* get_param(Module_Param_Name& param_name) const;
#else
  inline boolean is_present() const { return is_bound(); }
#endif

  void set_param(Module_Param& param);
  
  void encode_text(Text_Buf& text_buf) const;
  void decode_text(Text_Buf& text_buf);

private:
  void BER_encode_putbits(unsigned char *target,
                          unsigned int bitnum_start,
                          unsigned int bit_count) const;

  void BER_decode_getbits(const unsigned char *source,
                          size_t s_len, unsigned int& bitnum_start);

  void BER_decode_TLV_(const ASN_BER_TLV_t& p_tlv, unsigned L_form,
                       unsigned int& bitnum_start);
public:
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
    * another types during encoding. */
  int RAW_encode(const TTCN_Typedescriptor_t&, RAW_enc_tree&) const;
  /** Decodes the value of the variable according to the
   * TTCN_Typedescriptor_t. It must be public because called by
   * another types during encoding. Returns the number of decoded
   * bits. */
  int RAW_decode(const TTCN_Typedescriptor_t& , TTCN_Buffer&, int, raw_order_t,
     boolean no_err=FALSE, int sel_field=-1, boolean first_call=TRUE);

  int XER_encode(const XERdescriptor_t&, TTCN_Buffer&, unsigned int, unsigned int, int, embed_values_enc_struct_t*) const;
  int XER_decode(const XERdescriptor_t&, XmlReaderWrap& reader, unsigned int, unsigned int, embed_values_dec_struct_t*);
  
  /** Encodes accordingly to the JSON encoding rules.
    * Returns the length of the encoded data. */
  int JSON_encode(const TTCN_Typedescriptor_t&, JSON_Tokenizer&) const;
  
  /** Decodes accordingly to the JSON decoding rules.
    * Returns the length of the encoded data. */
  int JSON_decode(const TTCN_Typedescriptor_t&, JSON_Tokenizer&, boolean);
};

class BITSTRING_ELEMENT {
  boolean bound_flag;
  BITSTRING& str_val;
  int bit_pos;

public:
  BITSTRING_ELEMENT(boolean par_bound_flag, BITSTRING& par_str_val,
                    int par_bit_pos);

  BITSTRING_ELEMENT& operator=(const BITSTRING& other_value);
  BITSTRING_ELEMENT& operator=(const BITSTRING_ELEMENT& other_value);

  boolean operator==(const BITSTRING& other_value) const;
  boolean operator==(const BITSTRING_ELEMENT& other_value) const;

  boolean operator!=(const BITSTRING& other_value) const
  { return !(*this == other_value); }
  boolean operator!=(const BITSTRING_ELEMENT& other_value) const
  { return !(*this == other_value); }

  BITSTRING operator+(const BITSTRING& other_value) const;
  BITSTRING operator+(const BITSTRING_ELEMENT& other_value) const;
#ifdef TITAN_RUNTIME_2
  BITSTRING operator+(const OPTIONAL<BITSTRING>& other_value) const;
#endif

  BITSTRING operator~() const;
  BITSTRING operator&(const BITSTRING& other_value) const;
  BITSTRING operator&(const BITSTRING_ELEMENT& other_value) const;
  BITSTRING operator|(const BITSTRING& other_value) const;
  BITSTRING operator|(const BITSTRING_ELEMENT& other_value) const;
  BITSTRING operator^(const BITSTRING& other_value) const;
  BITSTRING operator^(const BITSTRING_ELEMENT& other_value) const;

  inline boolean is_bound() const { return bound_flag; }
  inline boolean is_present() const { return bound_flag; }
  inline boolean is_value() const { return bound_flag; }
  inline void must_bound(const char *err_msg) const
    { if (!bound_flag) TTCN_error("%s", err_msg); }

  boolean get_bit() const;

  void log() const;
};

/// bitstring template class
struct decmatch_struct;

class BITSTRING_template : public Restricted_Length_Template {
#ifdef __SUNPRO_CC
public:
#endif
  struct bitstring_pattern_struct;
private:
#ifdef TITAN_RUNTIME_2
  friend BITSTRING_template operator+(const BITSTRING& left_value,
    const BITSTRING_template& right_template);
  friend BITSTRING_template operator+(const BITSTRING_ELEMENT& left_value,
    const BITSTRING_template& right_template);
  friend BITSTRING_template operator+(const OPTIONAL<BITSTRING>& left_value,
    const BITSTRING_template& right_template);
  friend BITSTRING_template operator+(template_sel left_template_sel,
    const BITSTRING_template& right_template);
  friend BITSTRING_template operator+(const BITSTRING& left_value,
    template_sel right_template_sel);
  friend BITSTRING_template operator+(const BITSTRING_ELEMENT& left_value,
    template_sel right_template_sel);
  friend BITSTRING_template operator+(const OPTIONAL<BITSTRING>& left_value,
    template_sel right_template_sel);
  friend BITSTRING_template operator+(template_sel left_template_sel,
    const BITSTRING& right_value);
  friend BITSTRING_template operator+(template_sel left_template_sel,
    const BITSTRING_ELEMENT& right_value);
  friend BITSTRING_template operator+(template_sel left_template_sel,
    const OPTIONAL<BITSTRING>& right_value);
#endif
  
  BITSTRING single_value;
  union {
    struct {
      unsigned int n_values;
      BITSTRING_template *list_value;
    } value_list;
    bitstring_pattern_struct *pattern_value;
    decmatch_struct* dec_match;
  };

  void copy_template(const BITSTRING_template& other_value);
  static boolean match_pattern(const bitstring_pattern_struct *string_pattern,
    const BITSTRING::bitstring_struct *string_value);

#ifdef TITAN_RUNTIME_2
  void concat(Vector<unsigned char>& v) const;
  static void concat(Vector<unsigned char>& v, const BITSTRING& val);
  static void concat(Vector<unsigned char>& v, template_sel sel);
#endif
  
public:
  BITSTRING_template();
  BITSTRING_template(template_sel other_value);
  BITSTRING_template(const BITSTRING& other_value);
  BITSTRING_template(const BITSTRING_ELEMENT& other_value);
  BITSTRING_template(const OPTIONAL<BITSTRING>& other_value);
  BITSTRING_template(unsigned int n_elements,
    const unsigned char *pattern_elements);
  BITSTRING_template(const BITSTRING_template& other_value);
  ~BITSTRING_template();
  void clean_up();

  BITSTRING_template& operator=(template_sel other_value);
  BITSTRING_template& operator=(const BITSTRING& other_value);
  BITSTRING_template& operator=(const BITSTRING_ELEMENT& other_value);
  BITSTRING_template& operator=(const OPTIONAL<BITSTRING>& other_value);
  BITSTRING_template& operator=(const BITSTRING_template& other_value);
  
#ifdef TITAN_RUNTIME_2
  BITSTRING_template operator+(const BITSTRING_template& other_value) const;
  BITSTRING_template operator+(const BITSTRING& other_value) const;
  BITSTRING_template operator+(const BITSTRING_ELEMENT& other_value) const;
  BITSTRING_template operator+(const OPTIONAL<BITSTRING>& other_value) const;
  BITSTRING_template operator+(template_sel other_template_sel) const;
#endif

  BITSTRING_ELEMENT operator[](int index_value);
  BITSTRING_ELEMENT operator[](const INTEGER& index_value);
  const BITSTRING_ELEMENT operator[](int index_value) const;
  const BITSTRING_ELEMENT operator[](const INTEGER& index_value) const;

  boolean match(const BITSTRING& other_value, boolean legacy = FALSE) const;
  const BITSTRING& valueof() const;

  int lengthof() const;

  void set_type(template_sel template_type, unsigned int list_length = 0);
  BITSTRING_template& list_item(unsigned int list_index);
  
  void set_decmatch(Dec_Match_Interface* new_instance);
  
  void* get_decmatch_dec_res() const;
  const TTCN_Typedescriptor_t* get_decmatch_type_descr() const;

  void log() const;
  void log_match(const BITSTRING& match_value, boolean legacy = FALSE) const;

  void set_param(Module_Param& param);

  void encode_text(Text_Buf& text_buf) const;
  void decode_text(Text_Buf& text_buf);

  boolean is_present(boolean legacy = FALSE) const;
  boolean match_omit(boolean legacy = FALSE) const;
#ifdef TITAN_RUNTIME_2
  Module_Param* get_param(Module_Param_Name& param_name) const;
  void valueofv(Base_Type* value) const { *(static_cast<BITSTRING*>(value)) = valueof(); }
  void set_value(template_sel other_value) { *this = other_value; }
  void copy_value(const Base_Type* other_value) { *this = *(static_cast<const BITSTRING*>(other_value)); }
  Base_Template* clone() const { return new BITSTRING_template(*this); }
  const TTCN_Typedescriptor_t* get_descriptor() const { return &BITSTRING_descr_; }
  boolean matchv(const Base_Type* other_value, boolean legacy) const { return match(*(static_cast<const BITSTRING*>(other_value)), legacy); }
  void log_matchv(const Base_Type* match_value, boolean legacy) const  { log_match(*(static_cast<const BITSTRING*>(match_value)), legacy); }
#else
  void check_restriction(template_res t_res, const char* t_name=NULL, boolean legacy = FALSE) const;
#endif
};

#ifdef TITAN_RUNTIME_2
extern BITSTRING_template operator+(const BITSTRING& left_value,
  const BITSTRING_template& right_template);
extern BITSTRING_template operator+(const BITSTRING_ELEMENT& left_value,
  const BITSTRING_template& right_template);
extern BITSTRING_template operator+(const OPTIONAL<BITSTRING>& left_value,
  const BITSTRING_template& right_template);
extern BITSTRING_template operator+(template_sel left_template_sel,
  const BITSTRING_template& right_template);
extern BITSTRING_template operator+(const BITSTRING& left_value,
  template_sel right_template_sel);
extern BITSTRING_template operator+(const BITSTRING_ELEMENT& left_value,
  template_sel right_template_sel);
extern BITSTRING_template operator+(const OPTIONAL<BITSTRING>& left_value,
  template_sel right_template_sel);
extern BITSTRING_template operator+(template_sel left_template_sel,
  const BITSTRING& right_value);
extern BITSTRING_template operator+(template_sel left_template_sel,
  const BITSTRING_ELEMENT& right_value);
extern BITSTRING_template operator+(template_sel left_template_sel,
  const OPTIONAL<BITSTRING>& right_value);
#endif

#endif
