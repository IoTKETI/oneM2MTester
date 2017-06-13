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
 *   Ormandi, Matyas
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *
 ******************************************************************************/
#ifndef OPTIONAL_HH
#define OPTIONAL_HH

#include "Types.h"
#include "Param_Types.hh"
#include "Basetype.hh"
#include "Template.hh"
#include "BER.hh"
#include "Logger.hh"
#include "Encdec.hh"
#include "Textbuf.hh"
#include "Error.hh"
#include "Parameters.h"
#include "XER.hh"
#include "JSON.hh"
#include "XmlReader.hh"

enum optional_sel { OPTIONAL_UNBOUND, OPTIONAL_OMIT, OPTIONAL_PRESENT };

template <typename T_type>
class OPTIONAL : public Base_Type 
#ifdef TITAN_RUNTIME_2
  , public RefdIndexInterface
#endif
{
  /** The value, if present (owned by OPTIONAL) 
    * In Runtime2 the pointer is null, when the value is not present.
    * In Runtime1 its presence is indicated by the optional_selection member. */
  T_type *optional_value;
  
  /** Specifies the state of the optional field
    * @tricky In Runtime2 the optional value can be modified through parameter references,
    * in which case this member variable will not be updated. Always use the function
    * get_selection() instead of directly referencing this variable. */
  optional_sel optional_selection;
  
#ifdef TITAN_RUNTIME_2
  /** Stores the number of elements referenced by 'out' and 'inout' parameters,
    * if the optional field is a record of/set of/array (only in Runtime2).
    * If at least one element is referenced, the value must not be deleted. */
  int param_refs;
#endif

  /** Set the optional value to present.
   * If the value was already present, does nothing.
   * Else allocates a new (uninitialized) T-type.
   * @post \c optional_selection is OPTIONAL_PRESENT
   * @post \c optional_value is not NULL
   */
#ifdef TITAN_RUNTIME_2
public:
  virtual
#else
  inline
#endif
  void set_to_present() {
    if (optional_selection != OPTIONAL_PRESENT) {
      optional_selection = OPTIONAL_PRESENT;
#ifdef TITAN_RUNTIME_2
      if (optional_value == NULL)
#endif
        optional_value = new T_type;
    }
  }

  /** Set the optional value to omit.
   * If the value was present, frees it.
   * @post optional_selection is OPTIONAL_OMIT
   */
#ifdef TITAN_RUNTIME_2
public:
  virtual
#else
  inline
#endif
  void set_to_omit() {
#ifdef TITAN_RUNTIME_2
    if (is_present()) {
      if (param_refs > 0) {
        optional_value->clean_up();
      }
      else {
        delete optional_value;
        optional_value = NULL;
      }
    }
#else
    if (optional_selection == OPTIONAL_PRESENT) {
      delete optional_value;
    }
#endif
    optional_selection = OPTIONAL_OMIT;
  }

public:
  /// Default constructor creates an unbound object
  OPTIONAL() : optional_value(NULL), optional_selection(OPTIONAL_UNBOUND)
#ifdef TITAN_RUNTIME_2
  , param_refs(0)
#endif
  { }

  /// Construct an optional object set to omit.
  /// @p other_value must be OMIT_VALUE, or else dynamic testcase error.
  OPTIONAL(template_sel other_value);

  /// Copy constructor.
  /// @note Copying an unbound object creates another unbound object,
  /// without causing a dynamic test case immediately.
  OPTIONAL(const OPTIONAL& other_value);

  /// Construct from an optional of different type
  template <typename T_tmp>
  OPTIONAL(const OPTIONAL<T_tmp>& other_value);

  /// Construct from an object of different type
  template <typename T_tmp>
  OPTIONAL(const T_tmp& other_value)
  : optional_value(new T_type(other_value))
  ,  optional_selection(OPTIONAL_PRESENT)
#ifdef TITAN_RUNTIME_2
  ,  param_refs(0)
#endif
  { }

  ~OPTIONAL() {
#ifdef TITAN_RUNTIME_2
    if (NULL != optional_value)
#else
    if (optional_selection == OPTIONAL_PRESENT)
#endif
      delete optional_value; 
  }

  void clean_up();

  /// Set to omit.
  /// @p other_value must be OMIT_VALUE, or else dynamic testcase error.
  OPTIONAL& operator=(template_sel other_value);

  /// Copy assignment
  OPTIONAL& operator=(const OPTIONAL& other_value);

  /// Assign from an optional of another type
  template <typename T_tmp>
  OPTIONAL& operator=(const OPTIONAL<T_tmp>& other_value);

  /// Assign the value
  template <typename T_tmp>
  OPTIONAL& operator=(const T_tmp& other_value);

  boolean is_equal(template_sel other_value) const;
  boolean is_equal(const OPTIONAL& other_value) const;
  template <typename T_tmp>
  boolean is_equal(const OPTIONAL<T_tmp>& other_value) const;
  template <typename T_tmp>
  boolean is_equal(const T_tmp& other_value) const;

  inline boolean operator==(template_sel other_value) const
    { return is_equal(other_value); }
  inline boolean operator!=(template_sel other_value) const
    { return !is_equal(other_value); }
  inline boolean operator==(const OPTIONAL& other_value) const
    { return is_equal(other_value); }
  inline boolean operator!=(const OPTIONAL& other_value) const
    { return !is_equal(other_value); }
  template <typename T_tmp>
  inline boolean operator==(const T_tmp& other_value) const
    { return is_equal(other_value); }
  template <typename T_tmp>
  inline boolean operator!=(const T_tmp& other_value) const
    { return !is_equal(other_value); }
#if defined(__SUNPRO_CC) || defined(__clang__)
  /* Note: Without these functions the Sun Workshop Pro C++ compiler or clang reports
   * overloading ambiguity when comparing an optional charstring field with an
   * optional universal charstring. */
  template <typename T_tmp>
  inline boolean operator==(const OPTIONAL<T_tmp>& other_value) const
    { return is_equal(other_value); }
  template <typename T_tmp>
  inline boolean operator!=(const OPTIONAL<T_tmp>& other_value) const
    { return is_equal(other_value); }
#endif

#ifdef TITAN_RUNTIME_2
  boolean is_bound() const;
#else
  inline boolean is_bound() const { return optional_selection != OPTIONAL_UNBOUND; }
#endif
  boolean is_value() const
   { return optional_selection == OPTIONAL_PRESENT && optional_value->is_value(); }
  /** Whether the optional value is present.
   * @return \c true if optional_selection is OPTIONAL_PRESENT, else \c false */
#ifdef TITAN_RUNTIME_2
  boolean is_present() const;
#else
  inline boolean is_present() const { return optional_selection==OPTIONAL_PRESENT; }
#endif

#ifdef TITAN_RUNTIME_2
  /** @name override virtual functions of Base_Type
   * @{ */

  /** Return \c true (this \b is an optional field) */
  boolean is_optional() const { return TRUE; }

  /** Access the value for read/write
   *
   * @return a pointer to the (modifiable) value
   * @pre \p optional_selection must be \p OPTIONAL_PRESENT
   */
  Base_Type* get_opt_value();

  /** Access the value (read/only)
   *
   * @return a pointer to the (const) value
   * @pre \p optional_selection must be \p OPTIONAL_PRESENT
   */
  const Base_Type* get_opt_value() const;
  boolean is_seof() const;
  boolean is_equal(const Base_Type* other_value) const { return is_equal(*(static_cast<const OPTIONAL*>(other_value))); }
  void set_value(const Base_Type* other_value) { *this = *(static_cast<const OPTIONAL*>(other_value)); }
  Base_Type* clone() const { return new OPTIONAL(*this); }
  const TTCN_Typedescriptor_t* get_descriptor() const;
  /** @} */
#endif

  /** Whether the value is present.
   * Note: this is not the TTCN-3 ispresent(), kept for backward compatibility
   *       with the runtime and existing testports which use this version where 
   *       unbound errors are caught before causing more trouble
   *
   *  @return TRUE  if the value is present (optional_selection==OPTIONAL_PRESENT)
   *  @return FALSE if the value is not present (optional_selection==OPTIONAL_OMIT)
   *  @pre the value is bound (optional_selection!=OPTIONAL_UNBOUND)
   */
  boolean ispresent() const;
  
#ifdef TITAN_RUNTIME_2
  /** @tricky Calculates and returns the actual state of the optional object, 
    * not just the optional_selection member.
    * (Only needed in Runtime2, in Runtime1 optional_selection is always up to date.) */
  optional_sel get_selection() const;
#else
  inline optional_sel get_selection() const { return optional_selection; }
#endif

  void log() const;
  void set_param(Module_Param& param);
#ifdef TITAN_RUNTIME_2
  Module_Param* get_param(Module_Param_Name& param_name) const;
#endif
  void encode_text(Text_Buf& text_buf) const;
  void decode_text(Text_Buf& text_buf);

#ifdef TITAN_RUNTIME_2
  virtual int RAW_decode(const TTCN_Typedescriptor_t& td, TTCN_Buffer& buf, int limit,
    raw_order_t top_bit_ord, boolean no_err=FALSE, int sel_field=-1, boolean first_call=TRUE);
#endif

  int XER_encode(const XERdescriptor_t& p_td, TTCN_Buffer& buf, unsigned int flavor,
    unsigned int flavor2, int indent, embed_values_enc_struct_t* emb_val) const;
#ifdef TITAN_RUNTIME_2
  int XER_encode_negtest(const Erroneous_descriptor_t* p_err_descr,
    const XERdescriptor_t& p_td, TTCN_Buffer& p_buf, unsigned int flavor,
    unsigned int flavor2, int indent, embed_values_enc_struct_t* emb_val) const;
#endif
  /** Used during XML decoding, in case this object is an AnyElement field in a record.
    * Determines whether XER_decode() should be called or this field should be omitted.
    * The field should be omitted if:
    *  - the next element in the encoded XML is the next field in the record or
    *  - there are no more elements until the end of the record's XML element.
    * 
    * @param reader parses the encoded XML
    * @param next_field_name name of the next field in the record, or null if this is the last one
    * @param parent_tag_closed true, if the record's XML tag is closed (is an empty element)*/
  boolean XER_check_any_elem(XmlReaderWrap& reader, const char* next_field_name, boolean parent_tag_closed);
  int XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& reader,
    unsigned int flavor, unsigned int flavor2, embed_values_dec_struct_t* emb_val);

  char ** collect_ns(const XERdescriptor_t& p_td, size_t& num, bool& def_ns, unsigned int flavor = 0) const;

  operator T_type&();
  operator const T_type&() const;

  inline T_type& operator()() { return (T_type&)*this; }
  inline const T_type& operator()() const { return (const T_type&)*this; }

  ASN_BER_TLV_t* BER_encode_TLV(const TTCN_Typedescriptor_t& p_td,
                                unsigned p_coding) const;
#ifdef TITAN_RUNTIME_2
  ASN_BER_TLV_t* BER_encode_TLV_negtest(const Erroneous_descriptor_t* p_err_descr,
    const TTCN_Typedescriptor_t& p_td, unsigned p_coding) const;
#endif
  boolean BER_decode_TLV(const TTCN_Typedescriptor_t& p_td,
                         const ASN_BER_TLV_t& p_tlv, unsigned L_form);
  boolean BER_decode_isMyMsg(const TTCN_Typedescriptor_t& p_td,
                             const ASN_BER_TLV_t& p_tlv);
  void BER_decode_opentypes(TTCN_Type_list& p_typelist, unsigned L_form);

#ifdef TITAN_RUNTIME_2
  int TEXT_encode(const TTCN_Typedescriptor_t&, TTCN_Buffer&) const;
  int TEXT_encode_negtest(const Erroneous_descriptor_t* p_err_descr,
    const TTCN_Typedescriptor_t&, TTCN_Buffer&) const;
  int TEXT_decode(const TTCN_Typedescriptor_t&, TTCN_Buffer&, Limit_Token_List&,
                  boolean no_err=FALSE, boolean first_call=TRUE);
  int JSON_encode_negtest(const Erroneous_descriptor_t*,
                          const TTCN_Typedescriptor_t&, JSON_Tokenizer&) const;
#endif
  
  /** Encodes accordingly to the JSON encoding rules.
    * Returns the length of the encoded data. */
  int JSON_encode(const TTCN_Typedescriptor_t&, JSON_Tokenizer&) const;
  
  /** Decodes accordingly to the JSON encoding rules.
    * Returns the length of the decoded data. */
  int JSON_decode(const TTCN_Typedescriptor_t&, JSON_Tokenizer&, boolean);
  
#ifdef TITAN_RUNTIME_2
  /** Called before an element of an optional record of/set of is indexed and passed as an
    * 'inout' or 'out' parameter to a function (only in Runtime2).
    * Sets the optional value to present (this would be done by the indexing operation
    * anyway) and redirects the call to the optional value. */
  virtual void add_refd_index(int index);
  
  /** Called after an element of an optional record of/set of is passed as an
    * 'inout' or 'out' parameter to a function (only in Runtime2).
    * Redirects the call to the optional value. */
  virtual void remove_refd_index(int index);
#endif
};

#if HAVE_GCC(4,6)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
#endif

#ifdef TITAN_RUNTIME_2

template<typename T_type>
Base_Type* OPTIONAL<T_type>::get_opt_value()
{
#ifdef TITAN_RUNTIME_2
  if (!is_present())
#else
  if (optional_selection!=OPTIONAL_PRESENT)
#endif
    TTCN_error("Internal error: get_opt_value() called on a non-present optional field.");
  return optional_value;
}

template<typename T_type>
const Base_Type* OPTIONAL<T_type>::get_opt_value() const
{
#ifdef TITAN_RUNTIME_2
  if (!is_present())
#else
  if (optional_selection!=OPTIONAL_PRESENT)
#endif
    TTCN_error("Internal error: get_opt_value() const called on a non-present optional field.");
  return optional_value;
}

template<typename T_type>
boolean OPTIONAL<T_type>::is_seof() const
{
  return
#ifdef TITAN_RUNTIME_2
  (is_present())
#else
  (optional_selection==OPTIONAL_PRESENT)
#endif
  ? optional_value->is_seof() : T_type().is_seof();
}

template<typename T_type>
const TTCN_Typedescriptor_t* OPTIONAL<T_type>::get_descriptor() const
{
  return 
#ifdef TITAN_RUNTIME_2
  (is_present())
#else
  (optional_selection==OPTIONAL_PRESENT)
#endif
  ? optional_value->get_descriptor() : T_type().get_descriptor();
}

#endif

template<typename T_type>
OPTIONAL<T_type>::OPTIONAL(template_sel other_value)
  : optional_value(NULL), optional_selection(OPTIONAL_OMIT)
#ifdef TITAN_RUNTIME_2
  , param_refs(0)
#endif
{
  if (other_value != OMIT_VALUE)
    TTCN_error("Setting an optional field to an invalid value.");
}

template<typename T_type>
OPTIONAL<T_type>::OPTIONAL(const OPTIONAL& other_value)
  : Base_Type(other_value)
#ifdef TITAN_RUNTIME_2
  , RefdIndexInterface(other_value)
#endif     
  , optional_value(NULL)
  , optional_selection(other_value.optional_selection)
#ifdef TITAN_RUNTIME_2
  , param_refs(0)
#endif
{
  switch (other_value.optional_selection) {
  case OPTIONAL_PRESENT:
    optional_value = new T_type(*other_value.optional_value);
    break;
  case OPTIONAL_OMIT:
    break;
  case OPTIONAL_UNBOUND:
    break;
  }
}

template<typename T_type> template<typename T_tmp>
OPTIONAL<T_type>::OPTIONAL(const OPTIONAL<T_tmp>& other_value)
  : optional_value(NULL), optional_selection(other_value.get_selection())
#ifdef TITAN_RUNTIME_2
  , param_refs(0)
#endif
{
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    optional_value = new T_type((const T_tmp&)other_value);
    break;
  case OPTIONAL_OMIT:
    break;
  case OPTIONAL_UNBOUND:
    break;
  }
}

template<typename T_type>
void OPTIONAL<T_type>::clean_up()
{
#ifdef TITAN_RUNTIME_2
  if (is_present()) {
    if (param_refs > 0) {
      optional_value->clean_up();
    }
    else {
      delete optional_value;
      optional_value = NULL;
    }
  }
#else
  if (OPTIONAL_PRESENT == optional_selection) {
    delete optional_value;
  }
#endif
  optional_selection = OPTIONAL_UNBOUND;
}

template<typename T_type>
OPTIONAL<T_type>& OPTIONAL<T_type>::operator=(template_sel other_value)
{
  if (other_value != OMIT_VALUE)
    TTCN_error("Internal error: Setting an optional field to an invalid value.");
  set_to_omit();
  return *this;
}

template<typename T_type>
OPTIONAL<T_type>& OPTIONAL<T_type>::operator=(const OPTIONAL& other_value)
{
  switch (other_value.optional_selection) {
  case OPTIONAL_PRESENT:
#ifdef TITAN_RUNTIME_2
    if (NULL == optional_value) {
#else
    if (optional_selection != OPTIONAL_PRESENT) {
#endif
      optional_value = new T_type(*other_value.optional_value);
      optional_selection = OPTIONAL_PRESENT;
    } else *optional_value = *other_value.optional_value;
    break;
  case OPTIONAL_OMIT:
    if (&other_value != this) set_to_omit();
    break;
  case OPTIONAL_UNBOUND:
    clean_up();
    break;
  }
  return *this;
}

template<typename T_type> template <typename T_tmp>
OPTIONAL<T_type>&
OPTIONAL<T_type>::operator=(const OPTIONAL<T_tmp>& other_value)
{
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
#ifdef TITAN_RUNTIME_2
    if (NULL == optional_value) {
#else
    if (optional_selection != OPTIONAL_PRESENT) {
#endif
      optional_value = new T_type((const T_tmp&)other_value);
      optional_selection = OPTIONAL_PRESENT;
    } else *optional_value = (const T_tmp&)other_value;
    break;
  case OPTIONAL_OMIT:
    set_to_omit();
    break;
  case OPTIONAL_UNBOUND:
    clean_up();
    break;
  }
  return *this;
}

template<typename T_type> template <typename T_tmp>
OPTIONAL<T_type>&
OPTIONAL<T_type>::operator=(const T_tmp& other_value)
{
#ifdef TITAN_RUNTIME_2
  if (NULL == optional_value) {
#else
  if (optional_selection != OPTIONAL_PRESENT) {
#endif
    optional_value = new T_type(other_value);
    optional_selection = OPTIONAL_PRESENT;
  } else *optional_value = other_value;
  return *this;
}

template<typename T_type>
boolean OPTIONAL<T_type>::is_equal(template_sel other_value) const
{
#ifdef TITAN_RUNTIME_2
  if (!is_bound()) {
#else
  if (optional_selection == OPTIONAL_UNBOUND) {
#endif
    if (other_value == UNINITIALIZED_TEMPLATE) return TRUE;
    TTCN_error("The left operand of comparison is an unbound optional value.");
  }
  if (other_value != OMIT_VALUE) TTCN_error("Internal error: The right operand "
    "of comparison is an invalid value.");
  return
#ifdef TITAN_RUNTIME_2
  !is_present();
#else
  optional_selection == OPTIONAL_OMIT;
#endif
}

template<typename T_type>
boolean OPTIONAL<T_type>::is_equal(const OPTIONAL& other_value) const
{
#ifdef TITAN_RUNTIME_2
  if (!is_bound()) {
    if (!other_value.is_bound())
#else
  if (optional_selection == OPTIONAL_UNBOUND) {
    if (other_value.optional_selection == OPTIONAL_UNBOUND)
#endif
      return TRUE;
    TTCN_error("The left operand of "
    "comparison is an unbound optional value.");
  }
#ifdef TITAN_RUNTIME_2
  if (!other_value.is_bound())
#else
  if (other_value.optional_selection == OPTIONAL_UNBOUND)
#endif
    TTCN_error("The right operand of comparison is an unbound optional value.");
#ifdef TITAN_RUNTIME_2
  boolean present = is_present();
  if (present != other_value.is_present()) return FALSE;
  else if (present)
#else
  if (optional_selection != other_value.optional_selection) return FALSE;
  else if (optional_selection == OPTIONAL_PRESENT)
#endif
    return *optional_value == *other_value.optional_value;
  else return TRUE;
}

template<typename T_type> template <typename T_tmp>
boolean OPTIONAL<T_type>::is_equal(const T_tmp& other_value) const
{
#ifdef TITAN_RUNTIME_2
  switch (get_selection()) {
#else
  switch (optional_selection) {
#endif
  case OPTIONAL_PRESENT:
    return *optional_value == other_value;
  case OPTIONAL_OMIT:
    return FALSE;
  case OPTIONAL_UNBOUND:
    TTCN_error("The left operand of comparison is an unbound optional value.");
  }
  return FALSE;
}

template<typename T_type> template <typename T_tmp>
boolean OPTIONAL<T_type>::is_equal(const OPTIONAL<T_tmp>& other_value) const
{
#ifdef TITAN_RUNTIME_2
  if (!is_bound()) {
    if (!other_value.is_bound())
#else
  optional_sel other_selection = other_value.get_selection();
  if (optional_selection == OPTIONAL_UNBOUND) {
    if (other_selection == OPTIONAL_UNBOUND)
#endif
      return TRUE;
    TTCN_error("The left operand of "
    "comparison is an unbound optional value.");
  }
#ifdef TITAN_RUNTIME_2
  if (!other_value.is_bound())
#else
  if (other_selection == OPTIONAL_UNBOUND)
#endif
    TTCN_error("The right operand of comparison is an unbound optional value.");
#ifdef TITAN_RUNTIME_2
  boolean present = is_present();
  if (present != other_value.is_present()) return FALSE;
  else if (present)
#else
  if (optional_selection != other_selection) return FALSE;
  else if (optional_selection == OPTIONAL_PRESENT)
#endif
    return *optional_value == (const T_tmp&)other_value;
  else return TRUE;
}

#ifdef TITAN_RUNTIME_2
template<typename T_type>
boolean OPTIONAL<T_type>::is_bound() const
{
  switch (optional_selection) {
  case OPTIONAL_PRESENT:
  case OPTIONAL_OMIT:
    return TRUE;
  case OPTIONAL_UNBOUND:
  default:
    if (NULL != optional_value) {
      return optional_value->is_bound();
    }
    return FALSE;
  }
}

template<typename T_type>
boolean OPTIONAL<T_type>::is_present() const
{
  switch (optional_selection) {
  case OPTIONAL_PRESENT:
    return TRUE;
  case OPTIONAL_OMIT:
  case OPTIONAL_UNBOUND:
  default:
    if (NULL != optional_value) {
      return optional_value->is_bound();
    }
    return FALSE;
  }
}
#endif

template<typename T_type>
boolean OPTIONAL<T_type>::ispresent() const
{
  switch (optional_selection) {
  case OPTIONAL_PRESENT:
    return TRUE;
  case OPTIONAL_OMIT:
#ifdef TITAN_RUNTIME_2
    if (NULL != optional_value) {
      return optional_value->is_bound();
    }
#endif
    return FALSE;
  case OPTIONAL_UNBOUND:
#ifdef TITAN_RUNTIME_2
    if (NULL != optional_value && optional_value->is_bound()) {
      return TRUE;
    }
#endif
    TTCN_error("Using an unbound optional field.");
  }
  return FALSE;
}

#ifdef TITAN_RUNTIME_2
template<typename T_type>
optional_sel OPTIONAL<T_type>::get_selection() const
{
  if (is_present()) {
    return OPTIONAL_PRESENT;
  }
  if (is_bound()) {
    // not present, but bound => omit
    return OPTIONAL_OMIT;
  }
  return OPTIONAL_UNBOUND;
}
#endif

template<typename T_type>
void OPTIONAL<T_type>::log() const
{
#ifdef TITAN_RUNTIME_2
  switch (get_selection()) {
#else
  switch (optional_selection) {
#endif
  case OPTIONAL_PRESENT:
    optional_value->log();
    break;
  case OPTIONAL_OMIT:
    TTCN_Logger::log_event_str("omit");
    break;
  case OPTIONAL_UNBOUND:
    TTCN_Logger::log_event_unbound();
    break;
  }
}

template <typename T_type>
void OPTIONAL<T_type>::set_param(Module_Param& param) {
  if (param.get_type()==Module_Param::MP_Omit) {
    if (param.get_ifpresent()) param.error("An optional field of a record value cannot have an 'ifpresent' attribute");
    if (param.get_length_restriction()!=NULL) param.error("An optional field of a record value cannot have a length restriction");
    set_to_omit();
    return;
  }
  set_to_present();
  optional_value->set_param(param);
  if (!optional_value->is_bound()) {
    clean_up();
  }
}

#ifdef TITAN_RUNTIME_2
template <typename T_type>
Module_Param* OPTIONAL<T_type>::get_param(Module_Param_Name& param_name) const
{
#ifdef TITAN_RUNTIME_2
  switch (get_selection()) {
#else
  switch (optional_selection) {
#endif
  case OPTIONAL_PRESENT:
    return optional_value->get_param(param_name);
  case OPTIONAL_OMIT:
    return new Module_Param_Omit();
  case OPTIONAL_UNBOUND:
  default:
    return new Module_Param_Unbound();
  }
}
#endif

template<typename T_type>
void OPTIONAL<T_type>::encode_text(Text_Buf& text_buf) const
{
#ifdef TITAN_RUNTIME_2
  switch (get_selection()) {
#else
  switch (optional_selection) {
#endif
  case OPTIONAL_OMIT:
    text_buf.push_int((RInt)FALSE);
    break;
  case OPTIONAL_PRESENT:
    text_buf.push_int((RInt)TRUE);
    optional_value->encode_text(text_buf);
    break;
  case OPTIONAL_UNBOUND:
    TTCN_error("Text encoder: Encoding an unbound optional value.");
  }
}

template<typename T_type>
void OPTIONAL<T_type>::decode_text(Text_Buf& text_buf)
{
  if (text_buf.pull_int().get_val()) {
    set_to_present();
    optional_value->decode_text(text_buf);
  } else set_to_omit();
}

template<typename T_type>
int OPTIONAL<T_type>::JSON_encode(const TTCN_Typedescriptor_t& p_td, JSON_Tokenizer& p_tok) const
{
#ifdef TITAN_RUNTIME_2
  switch(get_selection()) {
#else
  switch(optional_selection) {
#endif
  case OPTIONAL_PRESENT:
    return optional_value->JSON_encode(p_td, p_tok);
  case OPTIONAL_OMIT:
    return p_tok.put_next_token(JSON_TOKEN_LITERAL_NULL, NULL);
  case OPTIONAL_UNBOUND:
  default:
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound optional value.");
    return -1;
  }
}

#ifdef TITAN_RUNTIME_2
template<typename T_type>
int OPTIONAL<T_type>::JSON_encode_negtest(const Erroneous_descriptor_t* p_err_descr,
                                        const TTCN_Typedescriptor_t& p_td,
                                        JSON_Tokenizer& p_tok) const 
{
  switch (get_selection()) {
  case OPTIONAL_PRESENT:
    return optional_value->JSON_encode_negtest(p_err_descr, p_td, p_tok);
  case OPTIONAL_OMIT:
    return p_tok.put_next_token(JSON_TOKEN_LITERAL_NULL, NULL);
  case OPTIONAL_UNBOUND:
  default:
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound optional value.");
    return -1;
  }
}
#endif

template<typename T_type>
int OPTIONAL<T_type>::JSON_decode(const TTCN_Typedescriptor_t& p_td, JSON_Tokenizer& p_tok, boolean p_silent)
{
  // try the optional value first
  set_to_present();
  size_t buf_pos = p_tok.get_buf_pos();
  int dec_len = optional_value->JSON_decode(p_td, p_tok, p_silent);
  if (JSON_ERROR_FATAL == dec_len) {
    if (p_silent) {
      clean_up();
    } else {
      set_to_omit();
    }
  }
  else if (JSON_ERROR_INVALID_TOKEN == dec_len) {
    // invalid token, rewind the buffer and check if it's a "null" (= omit)
    // this needs to be checked after the optional value, because it might also be
    // able to decode a "null" value
    p_tok.set_buf_pos(buf_pos);
    json_token_t token = JSON_TOKEN_NONE;
    dec_len = p_tok.get_next_token(&token, NULL, NULL);
    if (JSON_TOKEN_LITERAL_NULL == token) {
      set_to_omit();
    }
    else {
      // cannot get JSON_TOKEN_ERROR here, that was already checked by the optional value
      dec_len = JSON_ERROR_INVALID_TOKEN;
    }
  }
  return dec_len;
}

#ifdef TITAN_RUNTIME_2
template<typename T_type>
void OPTIONAL<T_type>::add_refd_index(int index)
{
  ++param_refs;
  set_to_present();
  RefdIndexInterface* refd_opt_val = dynamic_cast<RefdIndexInterface*>(optional_value);
  if (0 != refd_opt_val) {
    refd_opt_val->add_refd_index(index);
  }
}

template<typename T_type>
void OPTIONAL<T_type>::remove_refd_index(int index)
{
  --param_refs;
  RefdIndexInterface* refd_opt_val = dynamic_cast<RefdIndexInterface*>(optional_value);
  if (0 != refd_opt_val) {
    refd_opt_val->remove_refd_index(index);
  }
}
#endif

template<typename T_type>
OPTIONAL<T_type>::operator T_type&()
{
  set_to_present();
  return *optional_value;
}

template<typename T_type>
OPTIONAL<T_type>::operator const T_type&() const
{
#ifdef TITAN_RUNTIME_2
  if (!is_present())
#else
  if (optional_selection != OPTIONAL_PRESENT)
#endif
    TTCN_error("Using the value of an optional field containing omit.");
  return *optional_value;
}

template<typename T_type>
ASN_BER_TLV_t*
OPTIONAL<T_type>::BER_encode_TLV(const TTCN_Typedescriptor_t& p_td,
                                 unsigned p_coding) const
{
  BER_chk_descr(p_td);
#ifdef TITAN_RUNTIME_2
  switch (get_selection()) {
#else
  switch (optional_selection) {
#endif
  case OPTIONAL_PRESENT:
    return optional_value->BER_encode_TLV(p_td, p_coding);
  case OPTIONAL_OMIT:
    return ASN_BER_TLV_t::construct();
  case OPTIONAL_UNBOUND:
  default:
    return ASN_BER_V2TLV(BER_encode_chk_bound(FALSE), p_td, p_coding);
  }
}

#ifdef TITAN_RUNTIME_2
template<typename T_type>
ASN_BER_TLV_t*
OPTIONAL<T_type>::BER_encode_TLV_negtest(const Erroneous_descriptor_t* p_err_descr,
    const TTCN_Typedescriptor_t& p_td, unsigned p_coding) const
{
  BER_chk_descr(p_td);
#ifdef TITAN_RUNTIME_2
  switch (get_selection()) {
#else
  switch (optional_selection) {
#endif
  case OPTIONAL_PRESENT:
    return optional_value->BER_encode_TLV_negtest(p_err_descr, p_td, p_coding);
  case OPTIONAL_OMIT:
    return ASN_BER_TLV_t::construct();
  case OPTIONAL_UNBOUND:
  default:
    return ASN_BER_V2TLV(BER_encode_chk_bound(FALSE), p_td, p_coding);
  }
}

template<typename T_type>
int OPTIONAL<T_type>::RAW_decode(const TTCN_Typedescriptor_t& p_td,
  TTCN_Buffer&, int /* limit */, raw_order_t /* top_bit_ord */,
  boolean /* no_error */, int /* sel_field */, boolean /* first_call */ )
{
  TTCN_error("RAW decoding requested for optional type '%s'"
             " which has no RAW decoding method.",p_td.name);
  return 0;
}
#endif

template<typename T_type>
int
OPTIONAL<T_type>::XER_encode(const XERdescriptor_t& p_td, TTCN_Buffer& buf, unsigned int flavor,
  unsigned int flavor2, int indent, embed_values_enc_struct_t* emb_val) const
{
#ifdef TITAN_RUNTIME_2
  switch (get_selection()) {
#else
  switch (optional_selection) {
#endif
  case OPTIONAL_PRESENT:
    return optional_value->XER_encode(p_td, buf, flavor, flavor2, indent, emb_val);
  case OPTIONAL_OMIT:
    return 0; // nothing to do !
  case OPTIONAL_UNBOUND:
  default:
    TTCN_EncDec_ErrorContext::error(
      TTCN_EncDec::ET_UNBOUND, "Encoding an unbound optional value.");
    return 0;
  }
}

#ifdef TITAN_RUNTIME_2
template<typename T_type>
int
OPTIONAL<T_type>::XER_encode_negtest(const Erroneous_descriptor_t* p_err_descr,
  const XERdescriptor_t& p_td, TTCN_Buffer& buf, unsigned int flavor, unsigned int flavor2,
  int indent, embed_values_enc_struct_t* emb_val) const
{
  switch (get_selection()) {
  case OPTIONAL_PRESENT:
    return optional_value->XER_encode_negtest(p_err_descr, p_td, buf, flavor, flavor2, indent, emb_val);
  case OPTIONAL_OMIT:
    return 0; // nothing to do !
  case OPTIONAL_UNBOUND:
  default:
    TTCN_EncDec_ErrorContext::error(
      TTCN_EncDec::ET_UNBOUND, "Encoding an unbound optional value.");
    return 0;
  }
}
#endif


template<typename T_type>
bool
OPTIONAL<T_type>::XER_check_any_elem(XmlReaderWrap& reader, const char* next_field_name, boolean parent_tag_closed) 
{
  // If the record has no elements, then it can't have an AnyElement
  if (parent_tag_closed) {
    set_to_omit();
    return FALSE;
  }
  
  while (reader.Ok()) {
    // Leaving the record before finding an element -> no AnyElement
    if (XML_READER_TYPE_END_ELEMENT == reader.NodeType()) {
      set_to_omit();
      return FALSE;
    }
    if (XML_READER_TYPE_ELEMENT == reader.NodeType()) {
      // The first element found is either the next field's element or the AnyElement
      if (NULL != next_field_name && 
          0 == strcmp((const char*)reader.LocalName(), next_field_name)) {
        set_to_omit();
        return FALSE;
      }
      break;
    }
    reader.Read();
  }
   
  return TRUE;
}

template<typename T_type>
int
OPTIONAL<T_type>::XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& reader,
  unsigned int flavor, unsigned int flavor2, embed_values_dec_struct_t* emb_val)
{
  boolean exer  = is_exer(flavor);
  for (int success = reader.Ok(); success==1; success=reader.Read()) {
    int type = reader.NodeType();
    const char * name; // name of the optional field
    const char * ns_uri;

    if (exer && (p_td.xer_bits & XER_ATTRIBUTE)) {
      if (XML_READER_TYPE_ATTRIBUTE == type) {
        for (; success==1; success = reader.MoveToNextAttribute()) {
          if (!reader.IsNamespaceDecl()) break;
        }

        name = (const char*)reader.LocalName();
        if (!check_name(name, p_td, exer)) { // it's not us, bail
          break;
        }
        // we already checked for exer==1
        if (!check_namespace((const char*)reader.NamespaceUri(), p_td)) break;

        // set to omit if the attribute is empty
        const char * value = (const char *)reader.Value();
        if (strlen(value) == 0) {
          break;
        }
        
        set_to_present();
        optional_value->XER_decode(p_td, reader, flavor, flavor2, emb_val);
        goto finished;
      }
      else break;
    }
    else if(XML_READER_TYPE_ATTRIBUTE == type && (flavor & USE_NIL)){
        goto found_it;
    }
    else { // not attribute
      if (XML_READER_TYPE_ELEMENT == type) { // we are at an element
        name   = (const char*)reader.LocalName();
        ns_uri = (const char*)reader.NamespaceUri();
        if ((p_td.xer_bits & ANY_ELEMENT) || (exer && (flavor & USE_NIL))
            || ( (p_td.xer_bits & UNTAGGED) && !reader.IsEmptyElement())
          // If the optional field (a string) has anyElement, accept the element
          // regardless of its name. Else the name (and namespace) must match.
          || T_type::can_start(name, ns_uri, p_td, flavor, flavor2)) { // it is us
          found_it:
          set_to_present();
          //success = reader.Read(); // move to next thing TODO should it loop till an element ?
          // Pass XER_OPTIONAL to flavor, to sign that we are optional
          optional_value->XER_decode(p_td, reader, flavor | XER_OPTIONAL, flavor2, emb_val);
          if (!optional_value->is_bound()) {
            set_to_omit();
          }
        }
        else break; // it's not us, bail

        goto finished;
      }
      else if (XML_READER_TYPE_TEXT == type && (flavor & USE_NIL)) {
        goto found_it;
      }
      else if (XML_READER_TYPE_END_ELEMENT == type) {
        break;
      }
      // else circle around
    } // if attribute
  } // next
  set_to_omit();
  return 0;
finished:
  return 1;
}

template<typename T_type>
char ** OPTIONAL<T_type>::collect_ns(const XERdescriptor_t& p_td, size_t& num, bool& def_ns, unsigned int flavor) const {
#ifdef TITAN_RUNTIME_2
  switch (get_selection()) {
#else
  switch (optional_selection) {
#endif
  case OPTIONAL_PRESENT:
    return optional_value->collect_ns(p_td, num, def_ns, flavor);
  case OPTIONAL_OMIT:
    def_ns = FALSE;
    num = 0;
    return 0;
  case OPTIONAL_UNBOUND:
  default:
    TTCN_EncDec_ErrorContext::error(
      TTCN_EncDec::ET_UNBOUND, "Encoding an unbound value.");
    return 0;
  }
}

template<typename T_type>
boolean OPTIONAL<T_type>::BER_decode_TLV(const TTCN_Typedescriptor_t& p_td,
                                         const ASN_BER_TLV_t& p_tlv,
                                         unsigned L_form)
{
  BER_chk_descr(p_td);
  if (BER_decode_isMyMsg(p_td, p_tlv)) {
    return optional_value->BER_decode_TLV(p_td, p_tlv, L_form);
  } else {
    set_to_omit();
    return TRUE;
  }
}

template<typename T_type>
boolean OPTIONAL<T_type>::BER_decode_isMyMsg(const TTCN_Typedescriptor_t& p_td,
  const ASN_BER_TLV_t& p_tlv)
{
  set_to_present();
  return optional_value->BER_decode_isMyMsg(p_td, p_tlv);
}

template<typename T_type>
void OPTIONAL<T_type>::BER_decode_opentypes(TTCN_Type_list& p_typelist,
  unsigned L_form)
{
#ifdef TITAN_RUNTIME_2
  if (is_present()) {
    optional_selection = OPTIONAL_PRESENT;
#else
  if (optional_selection==OPTIONAL_PRESENT) {
#endif
    optional_value->BER_decode_opentypes(p_typelist, L_form);
  }
}

#ifdef TITAN_RUNTIME_2

template<typename T_type>
int OPTIONAL<T_type>::TEXT_encode(const TTCN_Typedescriptor_t& p_td,
                           TTCN_Buffer& buff) const
{
  if (is_present())
    return optional_value->TEXT_encode(p_td, buff);
  TTCN_error("Internal error: TEXT encoding an unbound/omit optional field.");
  return 0;
}

template<typename T_type>
int OPTIONAL<T_type>::TEXT_encode_negtest(const Erroneous_descriptor_t* p_err_descr,
  const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& buff) const
{
  if (is_present())
    return optional_value->TEXT_encode_negtest(p_err_descr, p_td, buff);
  TTCN_error("Internal error: TEXT encoding an unbound/omit optional field.");
  return 0;
}

template<typename T_type>
int OPTIONAL<T_type>::TEXT_decode(const TTCN_Typedescriptor_t& p_td,
  TTCN_Buffer& buff, Limit_Token_List& limit, boolean no_err, boolean first_call)
{
  set_to_present();
  return optional_value->TEXT_decode(p_td, buff, limit, no_err, first_call);
}

#endif

#if defined(__GNUC__) && __GNUC__ >= 3
/** Note: These functions allow most efficient operation by passing the left
 * operand OMIT_VALUE as value instead of constant reference.
 * However, with GCC 2.95.x the functions cause overloading ambiguities. */
template<typename T_type>
inline boolean operator==(template_sel left_value,
  const OPTIONAL<T_type>& right_value)
  { return right_value.is_equal(left_value); }

template<typename T_type>
inline boolean operator!=(template_sel left_value,
  const OPTIONAL<T_type>& right_value)
  { return !right_value.is_equal(left_value); }
#endif

template<typename T_left, typename T_right>
inline boolean operator==(const T_left& left_value,
  const OPTIONAL<T_right>& right_value)
  { return right_value.is_equal(left_value); }

template<typename T_left, typename T_right>
inline boolean operator!=(const T_left& left_value,
  const OPTIONAL<T_right>& right_value)
  { return !right_value.is_equal(left_value); }

#endif

#if HAVE_GCC(4,6)
#pragma GCC diagnostic pop
#endif
