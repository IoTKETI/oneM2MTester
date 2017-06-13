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
 *   Zalanyi, Balazs Andor
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#ifndef CHARSTRING_HH
#define CHARSTRING_HH

#include <sys/types.h>
#include <regex.h>

#include "Types.h"
#include "Basetype.hh"
#include "Template.hh"
#include "Param_Types.hh"

class INTEGER;
class BITSTRING;
class HEXSTRING;
class OCTETSTRING;
class CHARSTRING_ELEMENT;
class UNIVERSAL_CHARSTRING;
class UNIVERSAL_CHARSTRING_ELEMENT;
#ifdef TITAN_RUNTIME_2
class UNIVERSAL_CHARSTRING_template;
#endif

class Module_Param;

template<typename T>
class OPTIONAL;

/** Runtime class for character strings.
 *
 * Memory is reserved for the terminating null character
 * (not counted in the length).
 *
 * This class implements the following string types:
 *
 * - TTCN-3 \c charstring
 * - ASN.1 restricted character strings:
 *    - \c NumericString
 *    - \c PrintableString
 *    - \c IA5String
 *    - \c VisibleString.
 * - ASN.1 time types:
 *    - \c UTCTime
 *    - \c GeneralizedTime
 *
 */
class CHARSTRING : public Base_Type {

  friend class CHARSTRING_ELEMENT;
  friend class UNIVERSAL_CHARSTRING;
  friend class UNIVERSAL_CHARSTRING_ELEMENT;
  friend class TTCN_Buffer;
  friend class CHARSTRING_template;

  friend boolean operator==(const char* string_value,
                            const CHARSTRING& other_value);
  friend CHARSTRING operator+(const char* string_value,
                              const CHARSTRING& other_value);
  friend CHARSTRING operator+(const char* string_value,
                              const CHARSTRING_ELEMENT& other_value);

  friend CHARSTRING bit2str(const BITSTRING& value);
  friend CHARSTRING hex2str(const HEXSTRING& value);
  friend CHARSTRING oct2str(const OCTETSTRING& value);
  friend CHARSTRING unichar2char(const UNIVERSAL_CHARSTRING& value);
  friend CHARSTRING replace(const CHARSTRING& value, int index, int len,
		                    const CHARSTRING& repl);
  friend UNIVERSAL_CHARSTRING operator+(const universal_char& uchar_value,
    const UNIVERSAL_CHARSTRING_ELEMENT& other_value);
  friend UNIVERSAL_CHARSTRING operator+(const universal_char& uchar_value,
    const UNIVERSAL_CHARSTRING& other_value);
  friend boolean operator==(const universal_char& uchar_value,
    const UNIVERSAL_CHARSTRING& other_value);
  friend boolean operator==(const char *string_value,
    const UNIVERSAL_CHARSTRING_ELEMENT& other_value);
  friend UNIVERSAL_CHARSTRING operator+(const char *string_value,
    const UNIVERSAL_CHARSTRING& other_value);
  friend UNIVERSAL_CHARSTRING operator+(const char *string_value,
    const UNIVERSAL_CHARSTRING_ELEMENT& other_value);

  struct charstring_struct;
  charstring_struct *val_ptr;

  void init_struct(int n_chars);
  void copy_value();
  CHARSTRING(int n_chars);
  
  /** An extended version of set_param(), which also accepts string patterns if
    * the second parameter is set (needed by CHARSTRING_template to concatenate
    * string patterns). 
    * @return TRUE, if the module parameter was a string pattern, otherwise FALSE */
  boolean set_param_internal(Module_Param& param, boolean allow_pattern,
    boolean* is_nocase_pattern = NULL);
  
public:
  /** Construct an unbound CHARSTRING object */
  CHARSTRING();
  /** Construct a CHARSTRING of length 1 */
  CHARSTRING(char other_value);
  /** Construct a CHARSTRING from the given C string.
   * @param chars_ptr may be NULL, resulting in an empty CHARSTRING */
  CHARSTRING(const char* chars_ptr);
  /** Construct a CHARSTRING of a given length.
   *
   * CHARSTRING(0,NULL) is the most efficient way to a bound, empty string.
   *
   * @param n_chars number of characters
   * @param chars_ptr pointer to string (does not need to be 0-terminated)
   * @pre at least \p n_chars are readable from \p chars_ptr */
  CHARSTRING(int n_chars, const char* chars_ptr);
  /** Copy constructor */
  CHARSTRING(const CHARSTRING& other_value);
  /** Construct a CHARSTRING of length one */
  CHARSTRING(const CHARSTRING_ELEMENT& other_value);

  /** Destructor. Simply calls clean_up() */
  ~CHARSTRING();
  void clean_up();

  CHARSTRING& operator=(const char* other_value);
  CHARSTRING& operator=(const CHARSTRING& other_value);
  CHARSTRING& operator=(const CHARSTRING_ELEMENT& other_value);
  CHARSTRING& operator=(const UNIVERSAL_CHARSTRING& other_value);

  boolean operator==(const char* other_value) const;
  boolean operator==(const CHARSTRING& other_value) const;
  boolean operator==(const CHARSTRING_ELEMENT& other_value) const;
  boolean operator==(const UNIVERSAL_CHARSTRING& other_value) const;
  boolean operator==(const UNIVERSAL_CHARSTRING_ELEMENT& other_value) const;

  inline boolean operator!=(const char* other_value) const
  { return !(*this == other_value); }
  inline boolean operator!=(const CHARSTRING& other_value) const
  { return !(*this == other_value); }
  inline boolean operator!=(const CHARSTRING_ELEMENT& other_value) const
  { return !(*this == other_value); }

  CHARSTRING operator+(const char* other_value) const;
  CHARSTRING operator+(const CHARSTRING& other_value) const;
  CHARSTRING operator+(const CHARSTRING_ELEMENT& other_value) const;
#ifdef TITAN_RUNTIME_2
  CHARSTRING operator+(const OPTIONAL<CHARSTRING>& other_value) const;
#endif
  UNIVERSAL_CHARSTRING operator+(const UNIVERSAL_CHARSTRING& other_value) const;
  UNIVERSAL_CHARSTRING operator+
    (const UNIVERSAL_CHARSTRING_ELEMENT& other_value) const;
#ifdef TITAN_RUNTIME_2
  UNIVERSAL_CHARSTRING operator+(
    const OPTIONAL<UNIVERSAL_CHARSTRING>& other_value) const;
#endif

  CHARSTRING& operator+=(char other_value);
  CHARSTRING& operator+=(const char *other_value);
  CHARSTRING& operator+=(const CHARSTRING& other_value);
  CHARSTRING& operator+=(const CHARSTRING_ELEMENT& other_value);

  CHARSTRING operator<<=(int rotate_count) const;
  CHARSTRING operator<<=(const INTEGER& rotate_count) const;
  CHARSTRING operator>>=(int rotate_count) const;
  CHARSTRING operator>>=(const INTEGER& rotate_count) const;

  CHARSTRING_ELEMENT operator[](int index_value);
  CHARSTRING_ELEMENT operator[](const INTEGER& index_value);
  const CHARSTRING_ELEMENT operator[](int index_value) const;
  const CHARSTRING_ELEMENT operator[](const INTEGER& index_value) const;

  operator const char*() const;

  inline boolean is_bound() const { return val_ptr != NULL; }
  inline boolean is_value() const { return val_ptr != NULL; }
  inline void must_bound(const char *err_msg) const
    { if (val_ptr == NULL) TTCN_error("%s", err_msg); }

  int lengthof() const;

  void log() const;

#ifdef TITAN_RUNTIME_2
  boolean is_equal(const Base_Type* other_value) const { return *this == *(static_cast<const CHARSTRING*>(other_value)); }
  void set_value(const Base_Type* other_value) { *this = *(static_cast<const CHARSTRING*>(other_value)); }
  Base_Type* clone() const { return new CHARSTRING(*this); }
  const TTCN_Typedescriptor_t* get_descriptor() const { return &CHARSTRING_descr_; }
  Module_Param* get_param(Module_Param_Name& param_name) const;
#else
  inline boolean is_present() const { return is_bound(); }
#endif

  /** Initializes this object with a module parameter value or concatenates a module
    * parameter value to the end of this string.
    * @note UFT-8 strings will be decoded. If the decoding results in multi-octet
    * characters an error will be thrown. */
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
    * another types during encoding. Returns the length of encoded
    * data */
  int RAW_encode(const TTCN_Typedescriptor_t&, RAW_enc_tree&) const;
  /** Decodes the value of the variable according to the
    * TTCN_Typedescriptor_t. It must be public because called by
    * another types during encoding. Returns the number of decoded
    * bits */
  int RAW_decode(const TTCN_Typedescriptor_t&, TTCN_Buffer&, int, raw_order_t,
                 boolean no_err=FALSE, int sel_field=-1, boolean first_call=TRUE);
  int TEXT_encode(const TTCN_Typedescriptor_t&,
                 TTCN_Buffer&) const;
  int TEXT_decode(const TTCN_Typedescriptor_t&, TTCN_Buffer&,  Limit_Token_List&,
                  boolean no_err=FALSE, boolean first_call=TRUE);
  int XER_encode(const XERdescriptor_t&, TTCN_Buffer&, unsigned int, unsigned int, int, embed_values_enc_struct_t*) const;
  int XER_decode(const XERdescriptor_t&, XmlReaderWrap& reader, unsigned int, unsigned int, embed_values_dec_struct_t*);
  
  /** Returns the charstring in the format a string would appear in C or TTCN-3 code.
    * Inserts double quotation marks to the beginning and end of the string and
    * doubles the backlsashes in escaped characters.
    * Used for JSON encoding.
    * 
    * Example: "He said\nhis name was \"Al\"." -> "\"He said\\nhis name was \\\"Al\\\".\""
    * @note The returned character buffer needs to be freed after use. */
  char* to_JSON_string() const;
  
  /** Initializes the charstring with a JSON string value. 
    * The double quotation marks from the beginning and end of the JSON string are
    * removed and double-escaped characters are changed to simple-escaped ones. 
    * JSON characters stored as \u + 4 hex digits are converted to the characters
    * they represent.
    * Returns false if any non-charstring characters were found, otherwise true.
    * Used for JSON decoding.
    * 
    * Example: "\"He said\\nhis name was \\\"Al\\\".\"" -> "He said\nhis name was \"Al\"."
    * Example2: "\u0061" -> "a" 
    *
    * @param check_quotes turn the checking of double quotes (mentioned above) on or off */
  boolean from_JSON_string(const char* p_value, size_t p_value_len, boolean check_quotes);
  
  /** Encodes accordingly to the JSON encoding rules.
    * Returns the length of the encoded data. 
    * @note Since JSON has its own set of escaped characters, the ones in the
    * charstring need to be double-escaped. */
  int JSON_encode(const TTCN_Typedescriptor_t&, JSON_Tokenizer&) const;
  
  /** Decodes accordingly to the JSON encoding rules.
    * Returns the length of the decoded data. */
  int JSON_decode(const TTCN_Typedescriptor_t&, JSON_Tokenizer&, boolean);
  
#ifdef TITAN_RUNTIME_2
  virtual int encode_raw(TTCN_Buffer& p_buf) const;
  /** Adds this charstring to the end of a JSON buffer as raw data.
    * Used during the negative testing of the JSON encoder.
    * @return The number of bytes added. */
  int JSON_encode_negtest_raw(JSON_Tokenizer&) const;
#endif
};

class CHARSTRING_ELEMENT {
  boolean bound_flag;
  CHARSTRING& str_val;
  int char_pos;

public:
  CHARSTRING_ELEMENT(boolean par_bound_flag, CHARSTRING& par_str_val,
                     int par_char_pos);

  CHARSTRING_ELEMENT& operator=(const char* other_value);
  CHARSTRING_ELEMENT& operator=(const CHARSTRING& other_value);
  CHARSTRING_ELEMENT& operator=(const CHARSTRING_ELEMENT& other_value);

  boolean operator==(const char *other_value) const;
  boolean operator==(const CHARSTRING& other_value) const;
  boolean operator==(const CHARSTRING_ELEMENT& other_value) const;
  boolean operator==(const UNIVERSAL_CHARSTRING& other_value) const;
  boolean operator==(const UNIVERSAL_CHARSTRING_ELEMENT& other_value) const;

  inline boolean operator!=(const char *other_value) const
  { return !(*this == other_value); }
  inline boolean operator!=(const CHARSTRING& other_value) const
  { return !(*this == other_value); }
  inline boolean operator!=(const CHARSTRING_ELEMENT& other_value) const
  { return !(*this == other_value); }

  CHARSTRING operator+(const char *other_value) const;
  CHARSTRING operator+(const CHARSTRING& other_value) const;
  CHARSTRING operator+(const CHARSTRING_ELEMENT& other_value) const;
#ifdef TITAN_RUNTIME_2
  CHARSTRING operator+(const OPTIONAL<CHARSTRING>& other_value) const;
#endif
  UNIVERSAL_CHARSTRING operator+(const UNIVERSAL_CHARSTRING& other_value) const;
  UNIVERSAL_CHARSTRING operator+
    (const UNIVERSAL_CHARSTRING_ELEMENT& other_value) const;
#ifdef TITAN_RUNTIME_2
  UNIVERSAL_CHARSTRING operator+(
    const OPTIONAL<UNIVERSAL_CHARSTRING>& other_value) const;
#endif

  inline boolean is_bound() const { return bound_flag; }
  inline boolean is_present() const { return bound_flag; }
  inline boolean is_value() const { return bound_flag; }
  inline void must_bound(const char *err_msg) const
    { if (!bound_flag) TTCN_error("%s", err_msg); }

  char get_char() const;

  void log() const;
};

extern boolean operator==(const char* string_value,
                          const CHARSTRING& other_value);
extern boolean operator==(const char* string_value,
                          const CHARSTRING_ELEMENT& other_value);

inline boolean operator!=(const char* string_value,
                          const CHARSTRING& other_value)
{
  return !(string_value == other_value);
}

inline boolean operator!=(const char* string_value,
                          const CHARSTRING_ELEMENT& other_value)
{
  return !(string_value == other_value);
}

extern CHARSTRING operator+(const char* string_value,
                            const CHARSTRING& other_value);
extern CHARSTRING operator+(const char* string_value,
                            const CHARSTRING_ELEMENT& other_value);
#ifdef TITAN_RUNTIME_2
extern CHARSTRING operator+(const OPTIONAL<CHARSTRING>& left_value,
  const CHARSTRING& right_value);
extern CHARSTRING operator+(const OPTIONAL<CHARSTRING>& left_value,
  const CHARSTRING_ELEMENT& right_value);
extern UNIVERSAL_CHARSTRING operator+(const OPTIONAL<CHARSTRING>& left_value,
  const UNIVERSAL_CHARSTRING& right_value);
extern UNIVERSAL_CHARSTRING operator+(const OPTIONAL<CHARSTRING>& left_value,
  const UNIVERSAL_CHARSTRING_ELEMENT& right_value);
extern UNIVERSAL_CHARSTRING operator+(const OPTIONAL<CHARSTRING>& left_value,
  const OPTIONAL<UNIVERSAL_CHARSTRING>& right_value);
#endif

extern CHARSTRING operator<<=(const char *string_value,
                              const INTEGER& rotate_count);
extern CHARSTRING operator>>=(const char *string_value,
                              const INTEGER& rotate_count);

// charstring template class

struct unichar_decmatch_struct;

class CHARSTRING_template : public Restricted_Length_Template {

  friend class UNIVERSAL_CHARSTRING_template;
  
#ifdef TITAN_RUNTIME_2
  friend CHARSTRING_template operator+(const CHARSTRING& left_value,
    const CHARSTRING_template& right_template);
  friend CHARSTRING_template operator+(const CHARSTRING_ELEMENT& left_value,
    const CHARSTRING_template& right_template);
  friend CHARSTRING_template operator+(const OPTIONAL<CHARSTRING>& left_value,
    const CHARSTRING_template& right_template);
  friend UNIVERSAL_CHARSTRING_template operator+(
    const UNIVERSAL_CHARSTRING& left_value,
    const CHARSTRING_template& right_template);
  friend UNIVERSAL_CHARSTRING_template operator+(
    const UNIVERSAL_CHARSTRING_ELEMENT& left_value,
    const CHARSTRING_template& right_template);
  friend UNIVERSAL_CHARSTRING_template operator+(
    const OPTIONAL<UNIVERSAL_CHARSTRING>& left_value,
    const CHARSTRING_template& right_template);
  friend UNIVERSAL_CHARSTRING_template operator+(
    const CHARSTRING_template& left_template,
    const UNIVERSAL_CHARSTRING_template& right_template);
#endif

private:
  CHARSTRING single_value;
  union {
    struct {
      unsigned int n_values;
      CHARSTRING_template *list_value;
    } value_list;
    struct {
      boolean min_is_set, max_is_set;
      boolean min_is_exclusive, max_is_exclusive;
      char min_value, max_value;
    } value_range;
    mutable struct {
      boolean regexp_init;
      regex_t posix_regexp;
      boolean nocase;
    } pattern_value;
    unichar_decmatch_struct* dec_match;
  };

  void copy_template(const CHARSTRING_template& other_value);
  static void log_pattern(int n_chars, const char *chars_ptr, boolean nocase);

public:
  CHARSTRING_template();
  /** Constructor for any/omit/any-or-omit
   * @param other_value must be ANY_VALUE, OMIT_VALUE or ANY_OR_OMIT
   */
  CHARSTRING_template(template_sel other_value);
  /** Construct from a string
   * @post template_selection is SPECIFIC_VALUE
   * @param other_value
   */
  CHARSTRING_template(const CHARSTRING& other_value);
  /** Construct from a one-character string (result of CHARSTRING::op[])
   * @post template_selection is SPECIFIC_VALUE
   * @param other_value
   */
  CHARSTRING_template(const CHARSTRING_ELEMENT& other_value);
  /** Construct from an optional string
   * @post template_selection is SPECIFIC_VALUE or OMIT_VALUE
   * @param other_value
   */
  CHARSTRING_template(const OPTIONAL<CHARSTRING>& other_value);
  /** Constructor for STRING_PATTERN
   * @param p_sel must be STRING_PATTERN or else Dynamic Testcase Error
   * @param p_str pattern string
   * @param p_nocase case sensitivity (FALSE = case sensitive)
   */
  CHARSTRING_template(template_sel p_sel, const CHARSTRING& p_str,
    boolean p_nocase = FALSE);
  /// Copy constructor
  CHARSTRING_template(const CHARSTRING_template& other_value);

  ~CHARSTRING_template();
  void clean_up();

  CHARSTRING_template& operator=(template_sel other_value);
  CHARSTRING_template& operator=(const CHARSTRING& other_value);
  CHARSTRING_template& operator=(const CHARSTRING_ELEMENT& other_value);
  CHARSTRING_template& operator=(const OPTIONAL<CHARSTRING>& other_value);
  CHARSTRING_template& operator=(const CHARSTRING_template& other_value);
  
#ifdef TITAN_RUNTIME_2
  CHARSTRING_template operator+(const CHARSTRING_template& other_value) const;
  CHARSTRING_template operator+(const CHARSTRING& other_value) const;
  CHARSTRING_template operator+(const CHARSTRING_ELEMENT& other_value) const;
  CHARSTRING_template operator+(const OPTIONAL<CHARSTRING>& other_value) const;
  UNIVERSAL_CHARSTRING_template operator+(
    const UNIVERSAL_CHARSTRING& other_value) const;
  UNIVERSAL_CHARSTRING_template operator+(
    const UNIVERSAL_CHARSTRING_ELEMENT& other_value) const;
  UNIVERSAL_CHARSTRING_template operator+(
    const OPTIONAL<UNIVERSAL_CHARSTRING>& other_value) const;
#endif

  CHARSTRING_ELEMENT operator[](int index_value);
  CHARSTRING_ELEMENT operator[](const INTEGER& index_value);
  const CHARSTRING_ELEMENT operator[](int index_value) const;
  const CHARSTRING_ELEMENT operator[](const INTEGER& index_value) const;

  boolean match(const CHARSTRING& other_value, boolean legacy = FALSE) const;
  const CHARSTRING& valueof() const;

  int lengthof() const;

  void set_type(template_sel template_type, unsigned int list_length = 0);
  CHARSTRING_template& list_item(unsigned int list_index);
  
  void set_min(const CHARSTRING& min_value);
  void set_max(const CHARSTRING& max_value);
  void set_min_exclusive(boolean min_exclusive);
  void set_max_exclusive(boolean max_exclusive);
  
  void set_decmatch(Dec_Match_Interface* new_instance);
  
  void* get_decmatch_dec_res() const;
  const TTCN_Typedescriptor_t* get_decmatch_type_descr() const;

  void log() const;
  void log_match(const CHARSTRING& match_value, boolean legacy = FALSE) const;

  void set_param(Module_Param& param);

  void encode_text(Text_Buf& text_buf) const;
  void decode_text(Text_Buf& text_buf);

  boolean is_present(boolean legacy = FALSE) const;
  boolean match_omit(boolean legacy = FALSE) const;
#ifdef TITAN_RUNTIME_2
  Module_Param* get_param(Module_Param_Name& param_name) const;
  void valueofv(Base_Type* value) const { *(static_cast<CHARSTRING*>(value)) = valueof(); }
  void set_value(template_sel other_value) { *this = other_value; }
  void copy_value(const Base_Type* other_value) { *this = *(static_cast<const CHARSTRING*>(other_value)); }
  Base_Template* clone() const { return new CHARSTRING_template(*this); }
  const TTCN_Typedescriptor_t* get_descriptor() const { return &CHARSTRING_descr_; }
  boolean matchv(const Base_Type* other_value, boolean legacy) const { return match(*(static_cast<const CHARSTRING*>(other_value)), legacy); }
  void log_matchv(const Base_Type* match_value, boolean legacy) const  { log_match(*(static_cast<const CHARSTRING*>(match_value)), legacy); }
#else
  void check_restriction(template_res t_res, const char* t_name=NULL, boolean legacy = FALSE) const;
#endif

  /** Returns the single_value member
   *  @pre template_selection is SPECIFIC_VALUE or STRING_PATTERN */
  const CHARSTRING& get_single_value() const;
};

#ifdef TITAN_RUNTIME_2
extern CHARSTRING_template operator+(const CHARSTRING& left_value,
  const CHARSTRING_template& right_template);
extern CHARSTRING_template operator+(const CHARSTRING_ELEMENT& left_value,
  const CHARSTRING_template& right_template);
extern CHARSTRING_template operator+(const OPTIONAL<CHARSTRING>& left_value,
  const CHARSTRING_template& right_template);
extern UNIVERSAL_CHARSTRING_template operator+(
  const UNIVERSAL_CHARSTRING& left_value,
  const CHARSTRING_template& right_template);
extern UNIVERSAL_CHARSTRING_template operator+(
  const UNIVERSAL_CHARSTRING_ELEMENT& left_value,
  const CHARSTRING_template& right_template);
extern UNIVERSAL_CHARSTRING_template operator+(
  const OPTIONAL<UNIVERSAL_CHARSTRING>& left_value,
  const CHARSTRING_template& right_template);
#endif

typedef CHARSTRING NumericString;
typedef CHARSTRING PrintableString;
typedef CHARSTRING IA5String;
typedef CHARSTRING VisibleString;
typedef VisibleString ISO646String;

typedef CHARSTRING_template NumericString_template;
typedef CHARSTRING_template PrintableString_template;
typedef CHARSTRING_template IA5String_template;
typedef CHARSTRING_template VisibleString_template;
typedef VisibleString_template ISO646String_template;

typedef VisibleString ASN_GeneralizedTime;
typedef VisibleString ASN_UTCTime;

typedef VisibleString_template ASN_GeneralizedTime_template;
typedef VisibleString_template ASN_UTCTime_template;

// Only for backward compatibility
typedef CHARSTRING CHAR;
typedef CHARSTRING_template CHAR_template;

// TTCN_TYPE is a class that represents a ttcn-3 value or template
template<typename TTCN_TYPE>
CHARSTRING ttcn_to_string(const TTCN_TYPE& ttcn_data) {
  Logger_Format_Scope logger_format(TTCN_Logger::LF_TTCN);
  TTCN_Logger::begin_event_log2str();
  ttcn_data.log();
  return TTCN_Logger::end_event_log2str();
}

template<typename TTCN_TYPE>
void string_to_ttcn(const CHARSTRING& ttcn_string, TTCN_TYPE& ttcn_value) {
  Module_Param* parsed_mp = process_config_string2ttcn((const char*)ttcn_string, ttcn_value.is_component());
  try {
    Ttcn_String_Parsing ttcn_string_parsing;
    ttcn_value.set_param(*parsed_mp);
    delete parsed_mp;
  }
  catch (...) {
    delete parsed_mp;
    throw;
  }
}

#endif
