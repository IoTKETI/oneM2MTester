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
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#ifndef UNIVERSAL_CHARSTRING_HH
#define UNIVERSAL_CHARSTRING_HH

#include "Basetype.hh"
#include "Template.hh"
#include "Error.hh"
#include "CharCoding.hh"

#include <regex.h>
#include "Charstring.hh"

class CHARSTRING;
class CHARSTRING_ELEMENT;
class CHARSTRING_template;
class UNIVERSAL_CHARSTRING_ELEMENT;
class UNIVERSAL_CHARSTRING_template;
class INTEGER;
class Module_Param;

template<typename T>
class OPTIONAL;

extern boolean operator==(const universal_char& left_value,
  const universal_char& right_value);
extern boolean operator<(const universal_char& left_value,
  const universal_char& right_value);

/** Runtime class for Unicode character strings.
 *
 * This class implements the following string types:
 * - TTCN-3 \c universal \c charstring
 * - ASN.1
 *   - \c UTF8String
 *   - \c TeletexString
 *   - \c VideotexString
 *   - \c GraphicsString
 *   - \c GeneralString
 *   - \c UniversalString
 *   - \c BMPString
 *   - \c ObjectDescriptor
 *
 *  Note: an object of this class is effectively immutable after creation.
 *
 *  There is no terminating {0,0,0,0} at the end.
 */
class UNIVERSAL_CHARSTRING : public Base_Type {

  friend class CHARSTRING;
  friend class CHARSTRING_ELEMENT;
  friend class UNIVERSAL_CHARSTRING_ELEMENT;
  friend class TTCN_Buffer;
  friend class UNIVERSAL_CHARSTRING_template;

  friend UNIVERSAL_CHARSTRING oct2unichar(const OCTETSTRING& invalue,
                                          const CHARSTRING& string_encoding);
  friend UNIVERSAL_CHARSTRING oct2unichar(const OCTETSTRING& invalue);
  friend UNIVERSAL_CHARSTRING replace(const UNIVERSAL_CHARSTRING& value,
    int index, int len, const UNIVERSAL_CHARSTRING& repl);
  friend UNIVERSAL_CHARSTRING regexp(const UNIVERSAL_CHARSTRING& instr,
    const UNIVERSAL_CHARSTRING* expression_val,
    const UNIVERSAL_CHARSTRING_template* expression_tmpl,
    int groupno, boolean nocase);

  friend boolean operator==(const universal_char& uchar_value,
    const UNIVERSAL_CHARSTRING& other_value);
  friend UNIVERSAL_CHARSTRING operator+(const universal_char& uchar_value,
    const UNIVERSAL_CHARSTRING& other_value);
  friend boolean operator==(const char *string_value,
    const UNIVERSAL_CHARSTRING& other_value);
  friend UNIVERSAL_CHARSTRING operator+(const char *string_value,
    const UNIVERSAL_CHARSTRING& other_value);
  friend UNIVERSAL_CHARSTRING operator+(const char *string_value,
    const UNIVERSAL_CHARSTRING_ELEMENT& other_value);
  friend UNIVERSAL_CHARSTRING operator+(const universal_char& uchar_value,
    const UNIVERSAL_CHARSTRING_ELEMENT& other_value);
  friend boolean operator==(const char *string_value,
    const UNIVERSAL_CHARSTRING_ELEMENT& other_value);

  /** Internal data type, no user serviceable parts inside */
  struct universal_charstring_struct;
  /** Internal data */
  universal_charstring_struct *val_ptr;
  /** Character string values are stored in an optimal way */
  CHARSTRING cstr;
  boolean charstring;

  void init_struct(int n_uchars);
  void copy_value();
  UNIVERSAL_CHARSTRING(int n_uchars, boolean cstring = FALSE);
  
  /** An extended version of set_param(), which also accepts string patterns if
    * the second parameter is set (needed by UNIVERSAL_CHARSTRING_template to
    * concatenate string patterns). 
    * @return TRUE, if the module parameter was a string pattern, otherwise FALSE */
  boolean set_param_internal(Module_Param& param, boolean allow_pattern,
    boolean* is_nocase_pattern = NULL);

public:

  /** Construct an empty string */
  UNIVERSAL_CHARSTRING();
  /** Construct a string containing a single character.
   *
   * @param uc_group Unicode group (most significant part)
   * @param uc_plane Unicode plane
   * @param uc_row   Unicode row
   * @param uc_cell  Unicode cell (least significant)
   *
   */
  UNIVERSAL_CHARSTRING(unsigned char uc_group, unsigned char uc_plane,
    unsigned char uc_row, unsigned char uc_cell);
  /** Construct a string containing a single character
   *
   * @param other_value
   */
  UNIVERSAL_CHARSTRING(const universal_char& other_value);
  /** Construct a string from a given number of characters
   *
   * @param n_uchars
   * @param uchars_ptr
   */
  UNIVERSAL_CHARSTRING(int n_uchars, const universal_char *uchars_ptr);
  /** Construct a string from a C string
   *
   * @param chars_ptr pointer to the source string. A NULL pointer
   * will result in an empty UNIVERSAL_CHARSTRING.
   *
   * All characters in the string will have group==plane==row==0
   */
  UNIVERSAL_CHARSTRING(const char *chars_ptr);
  /** Construct a string from a known number of single-byte characters
   *
   * @param n_chars number of characters
   * @param chars_ptr pointer to the source string.
   *
   * @pre if n_chars > 0, chars_ptr must point to a valid string,
   * else an access violation will occur.
   *
   * All characters in the string will have group==plane==row==0
   */
  UNIVERSAL_CHARSTRING(int n_chars, const char *chars_ptr);
  /** Construct a copy of another CHARSTRING
   *
   * @param other_value
   */
  UNIVERSAL_CHARSTRING(const CHARSTRING& other_value);
  /** Construct a string containing a single CHARSTRING_ELEMENT
   *
   * @param other_value
   */
  UNIVERSAL_CHARSTRING(const CHARSTRING_ELEMENT& other_value);
  /** Copy constructor
   *
   * @param other_value
   */
  UNIVERSAL_CHARSTRING(const UNIVERSAL_CHARSTRING& other_value);
  /** Construct a string containing a single UNIVERSAL_CHARSTRING_ELEMENT
   *
   * @param other_value
   */
  UNIVERSAL_CHARSTRING(const UNIVERSAL_CHARSTRING_ELEMENT& other_value);

  ~UNIVERSAL_CHARSTRING();
  void clean_up();

  UNIVERSAL_CHARSTRING& operator=(const universal_char& other_value);
  UNIVERSAL_CHARSTRING& operator=(const char* other_value);
  UNIVERSAL_CHARSTRING& operator=(const CHARSTRING& other_value);
  UNIVERSAL_CHARSTRING& operator=(const CHARSTRING_ELEMENT& other_value);
  UNIVERSAL_CHARSTRING& operator=(const UNIVERSAL_CHARSTRING& other_value);
  UNIVERSAL_CHARSTRING& operator=(const UNIVERSAL_CHARSTRING_ELEMENT&
    other_value);

  /** @name Comparison operators
   * @{
   */
  boolean operator==(const universal_char& other_value) const;
  boolean operator==(const char* other_value) const;
  boolean operator==(const CHARSTRING& other_value) const;
  boolean operator==(const CHARSTRING_ELEMENT& other_value) const;
  boolean operator==(const UNIVERSAL_CHARSTRING& other_value) const;
  boolean operator==(const UNIVERSAL_CHARSTRING_ELEMENT&
    other_value) const;

  boolean operator!=(const universal_char& other_value) const
    { return !(*this == other_value); }
  boolean operator!=(const char* other_value) const
    { return !(*this == other_value); }
  boolean operator!=(const CHARSTRING& other_value) const
    { return !(*this == other_value); }
  boolean operator!=(const CHARSTRING_ELEMENT& other_value) const
    { return !(*this == other_value); }
  boolean operator!=(const UNIVERSAL_CHARSTRING& other_value) const
    { return !(*this == other_value); }
  boolean operator!=(const UNIVERSAL_CHARSTRING_ELEMENT&
    other_value) const { return !(*this == other_value); }
  /** @} */

  /** @name Concatenation
   *
   *
   * @{
   */
  UNIVERSAL_CHARSTRING operator+(const universal_char& other_value) const;
  UNIVERSAL_CHARSTRING operator+(const char* other_value) const;
  UNIVERSAL_CHARSTRING operator+(const CHARSTRING& other_value) const;
  UNIVERSAL_CHARSTRING operator+(const CHARSTRING_ELEMENT& other_value) const;
#ifdef TITAN_RUNTIME_2
  UNIVERSAL_CHARSTRING operator+(const OPTIONAL<CHARSTRING>& other_value) const;
#endif
  UNIVERSAL_CHARSTRING operator+(const UNIVERSAL_CHARSTRING& other_value) const;
  UNIVERSAL_CHARSTRING operator+(const UNIVERSAL_CHARSTRING_ELEMENT&
    other_value) const;
#ifdef TITAN_RUNTIME_2
  UNIVERSAL_CHARSTRING operator+(
    const OPTIONAL<UNIVERSAL_CHARSTRING>& other_value) const;
#endif
  /** @} */

  /** @name Rotation
   *  @{ */
  UNIVERSAL_CHARSTRING operator<<=(int rotate_count) const;
  UNIVERSAL_CHARSTRING operator<<=(const INTEGER& rotate_count) const;
  UNIVERSAL_CHARSTRING operator>>=(int rotate_count) const;
  UNIVERSAL_CHARSTRING operator>>=(const INTEGER& rotate_count) const;
  /** @} */

  /** @name Indexing
   *  @{ */
  UNIVERSAL_CHARSTRING_ELEMENT operator[](int index_value);
  UNIVERSAL_CHARSTRING_ELEMENT operator[](const INTEGER& index_value);
  const UNIVERSAL_CHARSTRING_ELEMENT operator[](int index_value) const;
  const UNIVERSAL_CHARSTRING_ELEMENT operator[]
    (const INTEGER& index_value) const;
  /** @} */

  operator const universal_char*() const;

  inline boolean is_bound() const {
    if (charstring) return cstr.is_bound();
    else return val_ptr != NULL; }
  inline boolean is_value() const {
    if (charstring) return cstr.is_value();
    else return val_ptr != NULL; }
  inline void must_bound(const char *err_msg) const
    { if (charstring) cstr.must_bound(err_msg);
      else if (val_ptr == NULL) TTCN_error("%s", err_msg); }

  int lengthof() const;
  
  /* returns the CHARSTRING representation of this. Quadruples are converted
   into the form: \q{group,plane,row,cell} */
  CHARSTRING get_stringRepr_for_pattern() const;
  
  void dump() const;

private:
  // convert this string to character string for pattern matching: ([A-P]{8})*
  char* convert_to_regexp_form() const;
  
  UNIVERSAL_CHARSTRING extract_matched_section(int start, int end) const;


  void convert_cstr_to_uni();
//checks if an valid BOM is at the head of the string. Returns the length of BOM
  int check_BOM(CharCoding::CharCodingType expected_coding, unsigned int length,
                     const unsigned char* ostr);
  
  /** Returns the universal charstring in the format a string would appear in C or TTCN-3 code.
    * Inserts double quotation marks to the begining and end of the string and
    * doubles the backlsashes in escaped characters.
    * Used for JSON encoding.
    * 
    * @param p_buf The buffer containing the universal charstring in UTF-8 encoding
    * 
    * Example: "He said\nhis name was \"Al\"." -> "\"He said\\nhis name was \\\"Al\\\".\""
    * Example2: char(0, 0, 1, 113) in TTCN-3 -> "\u0171"
    * @note The returned character buffer needs to be freed after use. */
  char* to_JSON_string(const TTCN_Buffer& p_buf) const;
  
  /** Converts the universal charstring from JSON string format to normal format. 
    * The double quotation marks from the beginning and end of the JSON string are
    * removed and double-escaped characters are changed to simple-escaped ones. 
    * JSON characters stored as \u + 4 hex digits are converted to the characters
    * they represent.
    * Returns false if any invalid characters were found, otherwise true.
    * Used for JSON decoding.
    * 
    * Example: "\"He said\\nhis name was \\\"Al\\\".\"" -> "He said\nhis name was \"Al\"."
    * Example2: "\u0061" -> "a" 
    *
    * @param check_quotes turn the checking of double quotes (mentioned above) on or off */
  boolean from_JSON_string(boolean check_quotes);
  
  /** Checks the coding of the characters in the received buffer, if they are 
    * encoded in UTF-8 this function will decode them. 
    * @return The universal charstring containing the decoded characters */
  static UNIVERSAL_CHARSTRING from_UTF8_buffer(TTCN_Buffer& p_buff);

public:

#ifdef TITAN_RUNTIME_2
  boolean is_equal(const Base_Type* other_value) const { return *this == *(static_cast<const UNIVERSAL_CHARSTRING*>(other_value)); }
  void set_value(const Base_Type* other_value) { *this = *(static_cast<const UNIVERSAL_CHARSTRING*>(other_value)); }
  Base_Type* clone() const { return new UNIVERSAL_CHARSTRING(*this); }
  const TTCN_Typedescriptor_t* get_descriptor() const { return &UNIVERSAL_CHARSTRING_descr_; }
  Module_Param* get_param(Module_Param_Name& param_name) const;
#else
  inline boolean is_present() const { return is_bound(); }
#endif

  void log() const;

  /** Initializes this object with a module parameter value or concatenates a module
    * parameter value to the end of this string.
    * @note UFT-8 strings (whose characters were not in quadruple notation) will 
    * be decoded */
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
  
  /** Encodes the value according to the TTCN_Typedescriptor_t.
    * It must be public because it can be called by other types during encoding.
    * Returns the length of encoded data */
  int RAW_encode(const TTCN_Typedescriptor_t&, RAW_enc_tree&) const;
  
  /** Decodes the value according to the TTCN_Typedescriptor_t.
    * It must be public because it can be called by other types during encoding.
    * Returns the number of decoded bits */
  int RAW_decode(const TTCN_Typedescriptor_t&, TTCN_Buffer&, int, raw_order_t,
                 boolean no_err=FALSE, int sel_field=-1, boolean first_call=TRUE);
  
  int TEXT_encode(const TTCN_Typedescriptor_t&,
                 TTCN_Buffer&) const;
  int TEXT_decode(const TTCN_Typedescriptor_t&, TTCN_Buffer&,  Limit_Token_List&,
                  boolean no_err=FALSE, boolean first_call=TRUE);
  int XER_encode(const XERdescriptor_t&, TTCN_Buffer&, unsigned int, unsigned int, int, embed_values_enc_struct_t*) const;
  int XER_decode(const XERdescriptor_t&, XmlReaderWrap& reader, unsigned int, unsigned int, embed_values_dec_struct_t*);
  /** Decodes UTF-8 into the internal representation (UCS4-BE)
   *
   * @param n_octets number of UTF-8 bytes (not characters)
   * @param octets_ptr pointer to the bytes
   *
   * @pre the UNIVERSAL_CHARSTRING is unbound or clean_up() has been called
   * (otherwise memory leak will occur)
   */
  void decode_utf8(int n_octets, const unsigned char *octets_ptr,
                   CharCoding::CharCodingType expected_coding = CharCoding::UTF_8, boolean checkBOM = FALSE);
  void decode_utf16(int n_octets, const unsigned char *octets_ptr,
                   CharCoding::CharCodingType expected_coding);
  void decode_utf32(int n_octets, const unsigned char *octets_ptr,
                   CharCoding::CharCodingType expected_coding);
  /** Appends the content of this string to the supplied buffer, in UTF-8
   *
   * @param[out] buf buffer to receive the encoded result
   */
  void encode_utf8(TTCN_Buffer& buf, boolean addBOM = FALSE) const;
  void encode_utf16(TTCN_Buffer& buf, CharCoding::CharCodingType expected_coding) const;
  void encode_utf32(TTCN_Buffer& buf, CharCoding::CharCodingType expected_coding) const;
  
  /** Encodes accordingly to the JSON encoding rules. 
    * The universal charstring will be encoded with UTF-8 before being added to
    * the JSON code.
    * Returns the length of the encoded data.  
    * @note Since JSON has its own set of escaped characters, the ones in the
    * universal charstring need to be double-escaped. */
  int JSON_encode(const TTCN_Typedescriptor_t&, JSON_Tokenizer&) const;
  
  /** Decodes accordingly to the JSON encoding rules.
    * Returns the length of the decoded data. */
  int JSON_decode(const TTCN_Typedescriptor_t&, JSON_Tokenizer&, boolean);
  
  /** Converts a string containing a character coding format to its corresponding
    * enum value.
    * @param coding_str string containing the coding format
    * @param context_str string containing the context this function was called
    * from (printed as part of an error message if the coding string is invalid) */
  static CharCoding::CharCodingType get_character_coding(const char* coding_str,
    const char* context_str);

private:
#ifdef TITAN_RUNTIME_2
  virtual int encode_raw(TTCN_Buffer& p_buf) const;
  /** Encodes this universal charstring with UTF-8 and adds the result to the end
    * of a JSON buffer as raw data.
    * Used during the negative testing of the JSON encoder.
    * @return The number of bytes added. */
  int JSON_encode_negtest_raw(JSON_Tokenizer&) const;
#endif
};

class UNIVERSAL_CHARSTRING_ELEMENT {
  friend class UNIVERSAL_CHARSTRING;
  friend UNIVERSAL_CHARSTRING operator+(const universal_char& uchar_value,
    const UNIVERSAL_CHARSTRING_ELEMENT& other_value);
  friend boolean operator==(const char *string_value,
    const UNIVERSAL_CHARSTRING_ELEMENT& other_value);
  friend UNIVERSAL_CHARSTRING operator+(const char *string_value,
    const UNIVERSAL_CHARSTRING_ELEMENT& other_value);
  friend UNIVERSAL_CHARSTRING CHARSTRING::operator+
    (const UNIVERSAL_CHARSTRING_ELEMENT& other_value) const;

  boolean bound_flag;
  UNIVERSAL_CHARSTRING& str_val;
  int uchar_pos;

public:
  UNIVERSAL_CHARSTRING_ELEMENT(boolean par_bound_flag,
    UNIVERSAL_CHARSTRING& par_str_val, int par_uchar_pos);

  UNIVERSAL_CHARSTRING_ELEMENT& operator=(const universal_char& other_value);
  UNIVERSAL_CHARSTRING_ELEMENT& operator=(const char* other_value);
  UNIVERSAL_CHARSTRING_ELEMENT& operator=(const CHARSTRING& other_value);
  UNIVERSAL_CHARSTRING_ELEMENT& operator=
    (const CHARSTRING_ELEMENT& other_value);
  UNIVERSAL_CHARSTRING_ELEMENT& operator=
    (const UNIVERSAL_CHARSTRING& other_value);
  UNIVERSAL_CHARSTRING_ELEMENT& operator=
    (const UNIVERSAL_CHARSTRING_ELEMENT& other_value);

  boolean operator==(const universal_char& other_value) const;
  boolean operator==(const char* other_value) const;
  boolean operator==(const CHARSTRING& other_value) const;
  boolean operator==(const CHARSTRING_ELEMENT& other_value) const;
  boolean operator==(const UNIVERSAL_CHARSTRING& other_value) const;
  boolean operator==(const UNIVERSAL_CHARSTRING_ELEMENT&
    other_value) const;

  boolean operator!=(const universal_char& other_value) const
    { return !(*this == other_value); }
  boolean operator!=(const char* other_value) const
    { return !(*this == other_value); }
  boolean operator!=(const CHARSTRING& other_value) const
    { return !(*this == other_value); }
  boolean operator!=(const CHARSTRING_ELEMENT& other_value) const
    { return !(*this == other_value); }
  boolean operator!=(const UNIVERSAL_CHARSTRING& other_value) const
    { return !(*this == other_value); }
  boolean operator!=(const UNIVERSAL_CHARSTRING_ELEMENT&
    other_value) const { return !(*this == other_value); }

  UNIVERSAL_CHARSTRING operator+(const universal_char& other_value) const;
  UNIVERSAL_CHARSTRING operator+(const char* other_value) const;
  UNIVERSAL_CHARSTRING operator+(const CHARSTRING& other_value) const;
  UNIVERSAL_CHARSTRING operator+(const CHARSTRING_ELEMENT& other_value) const;
#ifdef TITAN_RUNTIME_2
  UNIVERSAL_CHARSTRING operator+(const OPTIONAL<CHARSTRING>& other_value) const;
#endif
  UNIVERSAL_CHARSTRING operator+(const UNIVERSAL_CHARSTRING& other_value) const;
  UNIVERSAL_CHARSTRING operator+(const UNIVERSAL_CHARSTRING_ELEMENT&
    other_value) const;
#ifdef TITAN_RUNTIME_2
  UNIVERSAL_CHARSTRING operator+(
    const OPTIONAL<UNIVERSAL_CHARSTRING>& other_value) const;
#endif

  inline boolean is_bound() const { return bound_flag; }
  inline boolean is_present() const { return bound_flag; }
  inline boolean is_value() const { return bound_flag; }
  inline void must_bound(const char *err_msg) const
    { if (!bound_flag) TTCN_error("%s", err_msg); }

  const universal_char& get_uchar() const;
  void log() const;
};

extern boolean operator==(const universal_char& uchar_value,
  const UNIVERSAL_CHARSTRING& other_value);
extern boolean operator==(const universal_char& uchar_value,
  const UNIVERSAL_CHARSTRING_ELEMENT& other_value);

inline boolean operator!=(const universal_char& uchar_value,
  const UNIVERSAL_CHARSTRING& other_value)
{
  return !(uchar_value == other_value);
}

inline boolean operator!=(const universal_char& uchar_value,
  const UNIVERSAL_CHARSTRING_ELEMENT& other_value)
{
  return !(uchar_value == other_value);
}

inline boolean operator!=(const universal_char& uchar_value,
  const universal_char& other_value)
{
  return !(uchar_value == other_value);
}

extern UNIVERSAL_CHARSTRING operator+(const universal_char& uchar_value,
  const UNIVERSAL_CHARSTRING& other_value);
extern UNIVERSAL_CHARSTRING operator+(const universal_char& uchar_value,
  const UNIVERSAL_CHARSTRING_ELEMENT& other_value);
#ifdef TITAN_RUNTIME_2
extern UNIVERSAL_CHARSTRING operator+(
  const OPTIONAL<UNIVERSAL_CHARSTRING>& left_value,
  const UNIVERSAL_CHARSTRING& right_value);
extern UNIVERSAL_CHARSTRING operator+(
  const OPTIONAL<UNIVERSAL_CHARSTRING>& left_value,
  const UNIVERSAL_CHARSTRING_ELEMENT& right_value);
extern UNIVERSAL_CHARSTRING operator+(
  const OPTIONAL<UNIVERSAL_CHARSTRING>& left_value,
  const CHARSTRING& right_value);
extern UNIVERSAL_CHARSTRING operator+(
  const OPTIONAL<UNIVERSAL_CHARSTRING>& left_value,
  const CHARSTRING_ELEMENT& right_value);
extern UNIVERSAL_CHARSTRING operator+(
  const OPTIONAL<UNIVERSAL_CHARSTRING>& left_value,
  const OPTIONAL<CHARSTRING>& right_value);
#endif

extern boolean operator==(const char *string_value,
  const UNIVERSAL_CHARSTRING& other_value);
extern boolean operator==(const char *string_value,
  const UNIVERSAL_CHARSTRING_ELEMENT& other_value);

inline boolean operator!=(const char *string_value,
  const UNIVERSAL_CHARSTRING& other_value)
{
  return !(string_value == other_value);
}

inline boolean operator!=(const char *string_value,
  const UNIVERSAL_CHARSTRING_ELEMENT& other_value)
{
  return !(string_value == other_value);
}

extern UNIVERSAL_CHARSTRING operator+(const char *string_value,
  const UNIVERSAL_CHARSTRING& other_value);
extern UNIVERSAL_CHARSTRING operator+(const char *string_value,
  const UNIVERSAL_CHARSTRING_ELEMENT& other_value);


struct unichar_decmatch_struct;

class UNIVERSAL_CHARSTRING_template : public Restricted_Length_Template {
private:
  
#ifdef TITAN_RUNTIME_2
  friend UNIVERSAL_CHARSTRING_template operator+(
    const UNIVERSAL_CHARSTRING& left_value,
    const UNIVERSAL_CHARSTRING_template& right_template);
  friend UNIVERSAL_CHARSTRING_template operator+(
    const UNIVERSAL_CHARSTRING_ELEMENT& left_value,
    const UNIVERSAL_CHARSTRING_template& right_template);
  friend UNIVERSAL_CHARSTRING_template operator+(
    const OPTIONAL<UNIVERSAL_CHARSTRING>& left_value,
    const UNIVERSAL_CHARSTRING_template& right_template);
  friend UNIVERSAL_CHARSTRING_template operator+(const CHARSTRING& left_value,
    const UNIVERSAL_CHARSTRING_template& right_template);
  friend UNIVERSAL_CHARSTRING_template operator+(
    const CHARSTRING_ELEMENT& left_value,
    const UNIVERSAL_CHARSTRING_template& right_template);
  friend UNIVERSAL_CHARSTRING_template operator+(
    const OPTIONAL<CHARSTRING>& left_value,
    const UNIVERSAL_CHARSTRING_template& right_template);
  friend UNIVERSAL_CHARSTRING_template operator+(
    const CHARSTRING_template& left_template,
    const UNIVERSAL_CHARSTRING_template& right_template);
#endif
  
  UNIVERSAL_CHARSTRING single_value;
  CHARSTRING* pattern_string;
  union {
    struct {
      unsigned int n_values;
      UNIVERSAL_CHARSTRING_template *list_value;
    } value_list;
    struct {
      boolean min_is_set, max_is_set;
      boolean min_is_exclusive, max_is_exclusive;
      universal_char min_value, max_value;
    } value_range;
    mutable struct {
      boolean regexp_init;
      regex_t posix_regexp;
      boolean nocase;
    } pattern_value;
    unichar_decmatch_struct* dec_match;
  };

  void copy_template(const CHARSTRING_template& other_value);
  void copy_template(const UNIVERSAL_CHARSTRING_template& other_value);

public:
  UNIVERSAL_CHARSTRING_template();
  UNIVERSAL_CHARSTRING_template(template_sel other_value);
  UNIVERSAL_CHARSTRING_template(const CHARSTRING& other_value);
  UNIVERSAL_CHARSTRING_template(const UNIVERSAL_CHARSTRING& other_value);
  UNIVERSAL_CHARSTRING_template(const CHARSTRING_ELEMENT& other_value);
  UNIVERSAL_CHARSTRING_template
    (const UNIVERSAL_CHARSTRING_ELEMENT& other_value);
  UNIVERSAL_CHARSTRING_template(const OPTIONAL<CHARSTRING>& other_value);
  UNIVERSAL_CHARSTRING_template
    (const OPTIONAL<UNIVERSAL_CHARSTRING>& other_value);
  UNIVERSAL_CHARSTRING_template
    (const CHARSTRING_template& other_value);
  UNIVERSAL_CHARSTRING_template
    (const UNIVERSAL_CHARSTRING_template& other_value);
  UNIVERSAL_CHARSTRING_template(template_sel p_sel, const CHARSTRING& p_str,
    boolean p_nocase = FALSE);

  ~UNIVERSAL_CHARSTRING_template();
  void clean_up();

  UNIVERSAL_CHARSTRING_template& operator=(template_sel other_value);
  UNIVERSAL_CHARSTRING_template& operator=(const CHARSTRING& other_value);
  UNIVERSAL_CHARSTRING_template& operator=
    (const UNIVERSAL_CHARSTRING& other_value);
  UNIVERSAL_CHARSTRING_template& operator=
    (const CHARSTRING_ELEMENT& other_value);
  UNIVERSAL_CHARSTRING_template& operator=
    (const UNIVERSAL_CHARSTRING_ELEMENT& other_value);
  UNIVERSAL_CHARSTRING_template& operator=
    (const OPTIONAL<CHARSTRING>& other_value);
  UNIVERSAL_CHARSTRING_template& operator=
    (const OPTIONAL<UNIVERSAL_CHARSTRING>& other_value);
  UNIVERSAL_CHARSTRING_template& operator=
    (const CHARSTRING_template& other_value);
  UNIVERSAL_CHARSTRING_template& operator=
    (const UNIVERSAL_CHARSTRING_template& other_value);
  
#ifdef TITAN_RUNTIME_2
  UNIVERSAL_CHARSTRING_template operator+(
    const UNIVERSAL_CHARSTRING_template& other_value) const;
  UNIVERSAL_CHARSTRING_template operator+(
    const UNIVERSAL_CHARSTRING& other_value) const;
  UNIVERSAL_CHARSTRING_template operator+(
    const UNIVERSAL_CHARSTRING_ELEMENT& other_value) const;
  UNIVERSAL_CHARSTRING_template operator+(
    const OPTIONAL<UNIVERSAL_CHARSTRING>& other_value) const;
  UNIVERSAL_CHARSTRING_template operator+(const CHARSTRING& other_value) const;
  UNIVERSAL_CHARSTRING_template operator+(
    const CHARSTRING_ELEMENT& other_value) const;
  UNIVERSAL_CHARSTRING_template operator+(
    const OPTIONAL<CHARSTRING>& other_value) const;
  UNIVERSAL_CHARSTRING_template operator+(
    const CHARSTRING_template& other_value) const;
#endif
  
  UNIVERSAL_CHARSTRING_ELEMENT operator[](int index_value);
  UNIVERSAL_CHARSTRING_ELEMENT operator[](const INTEGER& index_value);
  const UNIVERSAL_CHARSTRING_ELEMENT operator[](int index_value) const;
  const UNIVERSAL_CHARSTRING_ELEMENT operator[]
    (const INTEGER& index_value) const;

  boolean match(const UNIVERSAL_CHARSTRING& other_value, boolean legacy = FALSE) const;
  const UNIVERSAL_CHARSTRING& valueof() const;

  int lengthof() const;

  void set_type(template_sel template_type, unsigned int list_length = 0);
  UNIVERSAL_CHARSTRING_template& list_item(unsigned int list_index);

  void set_min(const UNIVERSAL_CHARSTRING& min_value);
  void set_max(const UNIVERSAL_CHARSTRING& max_value);
  void set_min_exclusive(boolean min_exclusive);
  void set_max_exclusive(boolean max_exclusive);
  
  void set_decmatch(Dec_Match_Interface* new_instance, const char* coding_str = NULL);
  
  void* get_decmatch_dec_res() const;
  CharCoding::CharCodingType get_decmatch_str_enc() const;
  const TTCN_Typedescriptor_t* get_decmatch_type_descr() const;

  void log() const;
  void log_match(const UNIVERSAL_CHARSTRING& match_value, boolean legacy = FALSE) const;

  void set_param(Module_Param& param);

  void encode_text(Text_Buf& text_buf) const;
  void decode_text(Text_Buf& text_buf);

  boolean is_present(boolean legacy = FALSE) const;
  boolean match_omit(boolean legacy = FALSE) const;

#ifdef TITAN_RUNTIME_2
  Module_Param* get_param(Module_Param_Name& param_name) const;
  void valueofv(Base_Type* value) const { *(static_cast<UNIVERSAL_CHARSTRING*>(value)) = valueof(); }
  void set_value(template_sel other_value) { *this = other_value; }
  void copy_value(const Base_Type* other_value) { *this = *(static_cast<const UNIVERSAL_CHARSTRING*>(other_value)); }
  Base_Template* clone() const { return new UNIVERSAL_CHARSTRING_template(*this); }
  const TTCN_Typedescriptor_t* get_descriptor() const { return &UNIVERSAL_CHARSTRING_descr_; }
  boolean matchv(const Base_Type* other_value, boolean legacy) const { return match(*(static_cast<const UNIVERSAL_CHARSTRING*>(other_value)), legacy); }
  void log_matchv(const Base_Type* match_value, boolean legacy) const  { log_match(*(static_cast<const UNIVERSAL_CHARSTRING*>(match_value)), legacy); }
#else
  void check_restriction(template_res t_res, const char* t_name=NULL, boolean legacy = FALSE) const;
#endif

  const CHARSTRING& get_single_value() const;
};

#ifdef TITAN_RUNTIME_2
extern UNIVERSAL_CHARSTRING_template operator+(
  const UNIVERSAL_CHARSTRING& left_value,
  const UNIVERSAL_CHARSTRING_template& right_template);
extern UNIVERSAL_CHARSTRING_template operator+(
  const UNIVERSAL_CHARSTRING_ELEMENT& left_value,
  const UNIVERSAL_CHARSTRING_template& right_template);
extern UNIVERSAL_CHARSTRING_template operator+(
  const OPTIONAL<UNIVERSAL_CHARSTRING>& left_value,
  const UNIVERSAL_CHARSTRING_template& right_template);
extern UNIVERSAL_CHARSTRING_template operator+(const CHARSTRING& left_value,
  const UNIVERSAL_CHARSTRING_template& right_template);
extern UNIVERSAL_CHARSTRING_template operator+(const CHARSTRING_ELEMENT& left_value,
  const UNIVERSAL_CHARSTRING_template& right_template);
extern UNIVERSAL_CHARSTRING_template operator+(const OPTIONAL<CHARSTRING>& left_value,
  const UNIVERSAL_CHARSTRING_template& right_template);
extern UNIVERSAL_CHARSTRING_template operator+(const CHARSTRING_template& left_template,
  const UNIVERSAL_CHARSTRING_template& right_template);
#endif

typedef UNIVERSAL_CHARSTRING BMPString;
typedef UNIVERSAL_CHARSTRING UniversalString;
typedef UNIVERSAL_CHARSTRING UTF8String;
typedef UNIVERSAL_CHARSTRING TeletexString;
typedef TeletexString T61String;
typedef UNIVERSAL_CHARSTRING VideotexString;
typedef UNIVERSAL_CHARSTRING GraphicString;
typedef UNIVERSAL_CHARSTRING GeneralString;

typedef UNIVERSAL_CHARSTRING_template BMPString_template;
typedef UNIVERSAL_CHARSTRING_template UniversalString_template;
typedef UNIVERSAL_CHARSTRING_template UTF8String_template;
typedef UNIVERSAL_CHARSTRING_template TeletexString_template;
typedef TeletexString_template T61String_template;
typedef UNIVERSAL_CHARSTRING_template VideotexString_template;
typedef UNIVERSAL_CHARSTRING_template GraphicString_template;
typedef UNIVERSAL_CHARSTRING_template GeneralString_template;

typedef GraphicString ObjectDescriptor;
typedef GraphicString_template ObjectDescriptor_template;

OCTETSTRING TTCN_TeletexString_2_ISO2022(const TeletexString& p_s);
TeletexString TTCN_ISO2022_2_TeletexString(const OCTETSTRING& p_os);
OCTETSTRING TTCN_VideotexString_2_ISO2022(const VideotexString& p_s);
VideotexString TTCN_ISO2022_2_VideotexString(const OCTETSTRING& p_os);
OCTETSTRING TTCN_GraphicString_2_ISO2022(const GraphicString& p_s);
GraphicString TTCN_ISO2022_2_GraphicString(const OCTETSTRING& p_os);
OCTETSTRING TTCN_GeneralString_2_ISO2022(const GeneralString& p_s);
GeneralString TTCN_ISO2022_2_GeneralString(const OCTETSTRING& p_os);

#endif
