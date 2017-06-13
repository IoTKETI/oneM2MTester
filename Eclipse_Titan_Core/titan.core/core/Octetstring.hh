/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Baji, Laszlo
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
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#ifndef OCTETSTRING_HH
#define OCTETSTRING_HH

#include "Basetype.hh"
#include "Template.hh"
#include "Error.hh"
#ifdef TITAN_RUNTIME_2
#include "Vector.hh"
#endif

class INTEGER;
class BITSTRING;
class HEXSTRING;
class CHARSTRING;
class OCTETSTRING_ELEMENT;
class Module_Param;

template<typename T>
class OPTIONAL;

// octetstring value class

class OCTETSTRING : public Base_Type {

  friend class OCTETSTRING_ELEMENT;
  friend class OCTETSTRING_template;
  friend class TTCN_Buffer;

  friend OCTETSTRING int2oct(int value, int length);
  friend OCTETSTRING int2oct(const INTEGER& value, int length);
  friend OCTETSTRING str2oct(const CHARSTRING& value);
  friend OCTETSTRING bit2oct(const BITSTRING& value);
  friend OCTETSTRING hex2oct(const HEXSTRING& value);
  friend OCTETSTRING unichar2oct(const UNIVERSAL_CHARSTRING& invalue);
  friend OCTETSTRING unichar2oct(const UNIVERSAL_CHARSTRING& invalue, const CHARSTRING& string_encoding);
  friend OCTETSTRING replace(const OCTETSTRING& value, int index, int len,
                             const OCTETSTRING& repl);

protected: // for ASN_ANY which is derived from OCTETSTRING
  struct octetstring_struct;
  octetstring_struct *val_ptr;

  void init_struct(int n_octets);
  void copy_value();
  OCTETSTRING(int n_octets);

public:
  OCTETSTRING();
  OCTETSTRING(int n_octets, const unsigned char* octets_ptr);
  OCTETSTRING(const OCTETSTRING_ELEMENT& other_value);
  OCTETSTRING(const OCTETSTRING& other_value);
  ~OCTETSTRING();

  OCTETSTRING& operator=(const OCTETSTRING& other_value);
  OCTETSTRING& operator=(const OCTETSTRING_ELEMENT& other_value);

  boolean operator==(const OCTETSTRING& other_value) const;
  boolean operator==(const OCTETSTRING_ELEMENT& other_value) const;

  inline boolean operator!=(const OCTETSTRING& other_value) const
    { return !(*this == other_value); }
  inline boolean operator!=(const OCTETSTRING_ELEMENT& other_value) const
    { return !(*this == other_value); }

  OCTETSTRING operator+(const OCTETSTRING& other_value) const;
  OCTETSTRING operator+(const OCTETSTRING_ELEMENT& other_value) const;
#ifdef TITAN_RUNTIME_2
  OCTETSTRING operator+(const OPTIONAL<OCTETSTRING>& other_value) const;
#endif

  OCTETSTRING& operator+=(const OCTETSTRING& other_value);
  OCTETSTRING& operator+=(const OCTETSTRING_ELEMENT& other_value);

  OCTETSTRING operator~() const;
  OCTETSTRING operator&(const OCTETSTRING& other_value) const;
  OCTETSTRING operator&(const OCTETSTRING_ELEMENT& other_value) const;
  OCTETSTRING operator|(const OCTETSTRING& other_value) const;
  OCTETSTRING operator|(const OCTETSTRING_ELEMENT& other_value) const;
  OCTETSTRING operator^(const OCTETSTRING& other_value) const;
  OCTETSTRING operator^(const OCTETSTRING_ELEMENT& other_value) const;

  OCTETSTRING operator<<(int shift_count) const;
  OCTETSTRING operator<<(const INTEGER& shift_count) const;
  OCTETSTRING operator>>(int shift_count) const;
  OCTETSTRING operator>>(const INTEGER& shift_count) const;
  OCTETSTRING operator<<=(int rotate_count) const;
  OCTETSTRING operator<<=(const INTEGER& rotate_count) const;
  OCTETSTRING operator>>=(int rotate_count) const;
  OCTETSTRING operator>>=(const INTEGER& rotate_count) const;

  OCTETSTRING_ELEMENT operator[](int index_value);
  OCTETSTRING_ELEMENT operator[](const INTEGER& index_value);
  const OCTETSTRING_ELEMENT operator[](int index_value) const;
  const OCTETSTRING_ELEMENT operator[](const INTEGER& index_value) const;

  inline boolean is_bound() const { return val_ptr != NULL; }
  inline boolean is_value() const { return val_ptr != NULL; }
  inline void must_bound(const char *err_msg) const
    { if (val_ptr == NULL) TTCN_error("%s", err_msg); }
  void clean_up();

  int lengthof() const;
  operator const unsigned char*() const;
  void dump () const;

#ifdef TITAN_RUNTIME_2
  boolean is_equal(const Base_Type* other_value) const { return *this == *(static_cast<const OCTETSTRING*>(other_value)); }
  void set_value(const Base_Type* other_value) { *this = *(static_cast<const OCTETSTRING*>(other_value)); }
  Base_Type* clone() const { return new OCTETSTRING(*this); }
  const TTCN_Typedescriptor_t* get_descriptor() const { return &OCTETSTRING_descr_; }
  Module_Param* get_param(Module_Param_Name& param_name) const;
#else
  inline boolean is_present() const { return is_bound(); }
#endif

  void log() const;
  void set_param(Module_Param& param);

  void encode_text(Text_Buf& text_buf) const;
  void decode_text(Text_Buf& text_buf);

public:
  void encode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf,
              TTCN_EncDec::coding_t p_coding, ...) const;

  void decode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf,
              TTCN_EncDec::coding_t p_coding, ...);

  ASN_BER_TLV_t* BER_encode_TLV(const TTCN_Typedescriptor_t& p_td,
                                unsigned p_coding) const;
#ifdef TITAN_RUNTIME_2
  ASN_BER_TLV_t* BER_encode_negtest_raw() const;
  virtual int encode_raw(TTCN_Buffer& p_buf) const;
  virtual int RAW_encode_negtest_raw(RAW_enc_tree& p_myleaf) const;
  /** Adds this octetstring to the end of a JSON buffer as raw data.
    * Used during the negative testing of the JSON encoder.
    * @return The number of bytes added. */
  int JSON_encode_negtest_raw(JSON_Tokenizer&) const;
#endif
  boolean BER_decode_TLV(const TTCN_Typedescriptor_t& p_td,
                         const ASN_BER_TLV_t& p_tlv, unsigned L_form);

  /** Encodes the value of the variable according to the
    * TTCN_Typedescriptor_t. It must be public because called by
    * another types during encoding. Returns the length of encoded data*/
  int RAW_encode(const TTCN_Typedescriptor_t&, RAW_enc_tree&) const;
  /** Decodes the value of the variable according to the
   * TTCN_Typedescriptor_t. It must be public because called by
   * another types during encoding. Returns the number of decoded
   * bits. */
  int RAW_decode(const TTCN_Typedescriptor_t&, TTCN_Buffer& buff, int limit,
                 raw_order_t top_bit_ord, boolean no_err=FALSE,
                 int sel_field=-1, boolean first_call=TRUE);
  int TEXT_encode(const TTCN_Typedescriptor_t&,
                 TTCN_Buffer&) const;
  int TEXT_decode(const TTCN_Typedescriptor_t&, TTCN_Buffer&,  Limit_Token_List&,
                 boolean no_err=FALSE, boolean first_call=TRUE);
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


class OCTETSTRING_ELEMENT {
  boolean bound_flag;
  OCTETSTRING& str_val;
  int octet_pos;

public:
  OCTETSTRING_ELEMENT(boolean par_bound_flag, OCTETSTRING& par_str_val,
    int par_octet_pos);

  OCTETSTRING_ELEMENT& operator=(const OCTETSTRING& other_value);
  OCTETSTRING_ELEMENT& operator=(const OCTETSTRING_ELEMENT& other_value);

  boolean operator==(const OCTETSTRING& other_value) const;
  boolean operator==(const OCTETSTRING_ELEMENT& other_value) const;
  inline boolean operator!=(const OCTETSTRING& other_value) const
    { return !(*this == other_value); }
  inline boolean operator!=(const OCTETSTRING_ELEMENT& other_value) const
    { return !(*this == other_value); }

  OCTETSTRING operator+(const OCTETSTRING& other_value) const;
  OCTETSTRING operator+(const OCTETSTRING_ELEMENT& other_value) const;
#ifdef TITAN_RUNTIME_2
  OCTETSTRING operator+(const OPTIONAL<OCTETSTRING>& other_value) const;
#endif
  
  OCTETSTRING operator~() const;
  OCTETSTRING operator&(const OCTETSTRING& other_value) const;
  OCTETSTRING operator&(const OCTETSTRING_ELEMENT& other_value) const;
  OCTETSTRING operator|(const OCTETSTRING& other_value) const;
  OCTETSTRING operator|(const OCTETSTRING_ELEMENT& other_value) const;
  OCTETSTRING operator^(const OCTETSTRING& other_value) const;
  OCTETSTRING operator^(const OCTETSTRING_ELEMENT& other_value) const;

  inline boolean is_bound() const { return bound_flag; }
  inline boolean is_present() const { return bound_flag; }
  inline boolean is_value() const { return bound_flag; }
  inline void must_bound(const char *err_msg) const
    { if (!bound_flag) TTCN_error("%s", err_msg); }

  unsigned char get_octet() const;

  void log() const;
};

// octetstring template class

struct decmatch_struct;

class OCTETSTRING_template : public Restricted_Length_Template {
#ifdef __SUNPRO_CC
public:
#endif
  struct octetstring_pattern_struct;
private:

#ifdef TITAN_RUNTIME_2
  friend OCTETSTRING_template operator+(const OCTETSTRING& left_value,
    const OCTETSTRING_template& right_template);
  friend OCTETSTRING_template operator+(const OCTETSTRING_ELEMENT& left_value,
    const OCTETSTRING_template& right_template);
  friend OCTETSTRING_template operator+(const OPTIONAL<OCTETSTRING>& left_value,
    const OCTETSTRING_template& right_template);
  friend OCTETSTRING_template operator+(template_sel left_template_sel,
    const OCTETSTRING_template& right_template);
  friend OCTETSTRING_template operator+(const OCTETSTRING& left_value,
    template_sel right_template_sel);
  friend OCTETSTRING_template operator+(const OCTETSTRING_ELEMENT& left_value,
    template_sel right_template_sel);
  friend OCTETSTRING_template operator+(const OPTIONAL<OCTETSTRING>& left_value,
    template_sel right_template_sel);
  friend OCTETSTRING_template operator+(template_sel left_template_sel,
    const OCTETSTRING& right_value);
  friend OCTETSTRING_template operator+(template_sel left_template_sel,
    const OCTETSTRING_ELEMENT& right_value);
  friend OCTETSTRING_template operator+(template_sel left_template_sel,
    const OPTIONAL<OCTETSTRING>& right_value);
#endif
  
  OCTETSTRING single_value;
  union {
    struct {
      unsigned int n_values;
      OCTETSTRING_template *list_value;
    } value_list;
    octetstring_pattern_struct *pattern_value;
    decmatch_struct* dec_match;
  };

  void copy_template(const OCTETSTRING_template& other_value);
  static boolean match_pattern(const octetstring_pattern_struct *string_pattern,
    const OCTETSTRING::octetstring_struct *string_value);

#ifdef TITAN_RUNTIME_2
  void concat(Vector<unsigned short>& v) const;
  static void concat(Vector<unsigned short>& v, const OCTETSTRING& val);
  static void concat(Vector<unsigned short>& v, template_sel sel);
#endif
  
public:
  OCTETSTRING_template();
  OCTETSTRING_template(template_sel other_value);
  OCTETSTRING_template(const OCTETSTRING& other_value);
  OCTETSTRING_template(const OCTETSTRING_ELEMENT& other_value);
  OCTETSTRING_template(const OPTIONAL<OCTETSTRING>& other_value);
  OCTETSTRING_template(unsigned int n_elements,
    const unsigned short *pattern_elements);
  OCTETSTRING_template(const OCTETSTRING_template& other_value);
  ~OCTETSTRING_template();
  void clean_up();

  OCTETSTRING_template& operator=(template_sel other_value);
  OCTETSTRING_template& operator=(const OCTETSTRING& other_value);
  OCTETSTRING_template& operator=(const OCTETSTRING_ELEMENT& other_value);
  OCTETSTRING_template& operator=(const OPTIONAL<OCTETSTRING>& other_value);
  OCTETSTRING_template& operator=(const OCTETSTRING_template& other_value);
  
#ifdef TITAN_RUNTIME_2
  OCTETSTRING_template operator+(const OCTETSTRING_template& other_value) const;
  OCTETSTRING_template operator+(const OCTETSTRING& other_value) const;
  OCTETSTRING_template operator+(const OCTETSTRING_ELEMENT& other_value) const;
  OCTETSTRING_template operator+(const OPTIONAL<OCTETSTRING>& other_value) const;
  OCTETSTRING_template operator+(template_sel other_template_sel) const;
#endif

  OCTETSTRING_ELEMENT operator[](int index_value);
  OCTETSTRING_ELEMENT operator[](const INTEGER& index_value);
  const OCTETSTRING_ELEMENT operator[](int index_value) const;
  const OCTETSTRING_ELEMENT operator[](const INTEGER& index_value) const;

  boolean match(const OCTETSTRING& other_value, boolean legacy = FALSE) const;
  const OCTETSTRING& valueof() const;

  int lengthof() const;

  void set_type(template_sel template_type, unsigned int list_length = 0);
  OCTETSTRING_template& list_item(unsigned int list_index);
  
  void set_decmatch(Dec_Match_Interface* new_instance);
  
  void* get_decmatch_dec_res() const;
  const TTCN_Typedescriptor_t* get_decmatch_type_descr() const;

  void log() const;
  void log_match(const OCTETSTRING& match_value, boolean legacy = FALSE) const;

  void set_param(Module_Param& param);

  void encode_text(Text_Buf& text_buf) const;
  void decode_text(Text_Buf& text_buf);

  boolean is_present(boolean legacy = FALSE) const;
  boolean match_omit(boolean legacy = FALSE) const;
#ifdef TITAN_RUNTIME_2
  Module_Param* get_param(Module_Param_Name& param_name) const;
  void valueofv(Base_Type* value) const { *(static_cast<OCTETSTRING*>(value)) = valueof(); }
  void set_value(template_sel other_value) { *this = other_value; }
  void copy_value(const Base_Type* other_value) { *this = *(static_cast<const OCTETSTRING*>(other_value)); }
  Base_Template* clone() const { return new OCTETSTRING_template(*this); }
  const TTCN_Typedescriptor_t* get_descriptor() const { return &OCTETSTRING_descr_; }
  boolean matchv(const Base_Type* other_value, boolean legacy) const { return match(*(static_cast<const OCTETSTRING*>(other_value)), legacy); }
  void log_matchv(const Base_Type* match_value, boolean legacy) const  { log_match(*(static_cast<const OCTETSTRING*>(match_value)), legacy); }
#else
  void check_restriction(template_res t_res, const char* t_name=NULL, boolean legacy = FALSE) const;
#endif
};

#ifdef TITAN_RUNTIME_2
extern OCTETSTRING_template operator+(const OCTETSTRING& left_value,
  const OCTETSTRING_template& right_template);
extern OCTETSTRING_template operator+(const OCTETSTRING_ELEMENT& left_value,
  const OCTETSTRING_template& right_template);
extern OCTETSTRING_template operator+(const OPTIONAL<OCTETSTRING>& left_value,
  const OCTETSTRING_template& right_template);
extern OCTETSTRING_template operator+(template_sel left_template_sel,
  const OCTETSTRING_template& right_template);
extern OCTETSTRING_template operator+(const OCTETSTRING& left_value,
  template_sel right_template_sel);
extern OCTETSTRING_template operator+(const OCTETSTRING_ELEMENT& left_value,
  template_sel right_template_sel);
extern OCTETSTRING_template operator+(const OPTIONAL<OCTETSTRING>& left_value,
  template_sel right_template_sel);
extern OCTETSTRING_template operator+(template_sel left_template_sel,
  const OCTETSTRING& right_value);
extern OCTETSTRING_template operator+(template_sel left_template_sel,
  const OCTETSTRING_ELEMENT& right_value);
extern OCTETSTRING_template operator+(template_sel left_template_sel,
  const OPTIONAL<OCTETSTRING>& right_value);
#endif

#endif
