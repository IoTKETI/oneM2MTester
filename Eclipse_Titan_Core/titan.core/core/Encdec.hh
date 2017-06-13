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
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *
 ******************************************************************************/
#ifndef ENCDEC_HH
#define ENCDEC_HH

#include "Types.h"

typedef const unsigned char cbyte;


class OCTETSTRING;
class CHARSTRING;
class UNIVERSAL_CHARSTRING;
struct RAW_coding_par;

/** Endianness indicator */
enum raw_order_t {
  ORDER_MSB,
  ORDER_LSB
};

/**
  * Static class which encapsulates the stuff related to
  * encoding/decoding.
  */
class TTCN_EncDec {
public:
  /** Coding types. */
  enum coding_t {
    CT_BER,  /**< BER */
    CT_PER,  /**< PER */
    CT_RAW,  /**< RAW */
    CT_TEXT, /**< TEXT */
    CT_XER,   /**< XER */
    CT_JSON  /**< JSON */
  };
  /** Error type enum type. If you want to add new values, please put
    * them before ET_ALL. Values must be consecutive, starting at zero.
    * When adding a new value, a corresponding string (without "ET_")
    * must be added to compiler2/ttcn3/Ttcnstuff.cc */
  enum error_type_t {
    ET_UNDEF = 0,     /**< Undefined error. */
    ET_UNBOUND = 1,   /**< Encoding of an unbound value. */
    ET_INCOMPL_ANY,   /**< Encoding of an ASN ANY value which does not
                           contain a valid BER TLV. */
    ET_ENC_ENUM,      /**< Encoding of an unknown enumerated value. */
    ET_INCOMPL_MSG,   /**< Decode error: incomplete message. */
    ET_LEN_FORM,      /**< During decoding: the received message has a
                           non-acceptable length form (BER). 5*/
    ET_INVAL_MSG,     /**< Decode error: invalid message. */
    ET_REPR,          /**< Representation error, e.g.: internal
                           representation of integral numbers. */
    ET_CONSTRAINT,    /**< The value breaks some constraint. */
    ET_TAG,           /**< During decoding: unexpected tag. */
    ET_SUPERFL,       /**< During decoding: superfluous part
                           detected. In case of BER, this can be
                           superfluous TLV at the end of a constructed
                           TLV. 10 */
    ET_EXTENSION,     /**< During decoding: there was something in the
                           extension (e.g.: in ASN.1 ellipsis). */
    ET_DEC_ENUM,      /**< During decoding of an (inextensible)
                           enumerated value: unknown value received. */
    ET_DEC_DUPFLD,    /**< During decoding: duplicated field. For
                           example, while decoding a SET type. */
    ET_DEC_MISSFLD,   /**< During decoding: missing field. For example,
                           while decoding a SET type. */
    ET_DEC_OPENTYPE,  /**< Cannot decode an opentype (broken component
                           relation constraint). 15 */
    ET_DEC_UCSTR,     /**< During decoding a universal charstring:
                           something went wrong. */
    ET_LEN_ERR,       /**< During RAW encoding: the specified field
                           length is not enough to encode the value of
                           the field. During RAW decoding: the available
                           number of bits is less than it needed to
                           decode the field. */
    ET_SIGN_ERR,      /**< Unsigned encoding of a negative number. */
    ET_INCOMP_ORDER,  /**< Incompatible combination of orders attribute
                           for RAW coding. */
    ET_TOKEN_ERR,     /**< During TEXT decoding the specified token is
                           not found. 20 */
    ET_LOG_MATCHING,  /**< During TEXT decoding the matching is logged
                           if the behavior was set to EB_WARNING. */
    ET_FLOAT_TR,      /**< The float value will be truncated during
                           double -> single precision conversion */
    ET_FLOAT_NAN,     /**< Not a Number has been received */
    ET_OMITTED_TAG,   /**< During encoding the key of a tag references
                           an optional field with omitted value */
    ET_NEGTEST_CONFL, /**< Contradictory negative testing and RAW attributes. */
    ET_ALL,           /**< Used only when setting error behavior. 25 */
    ET_INTERNAL,      /**< Internal error. Error behavior cannot be set
                           for this. */
    ET_NONE           /**< There was no error. */
  };
  /** Error behavior enum type. */
  enum error_behavior_t {
    EB_DEFAULT, /**< Used only when setting error behavior. */
    EB_ERROR,   /**< Raises an error. */
    EB_WARNING, /**< Logs the error but continues the activity. */
    EB_IGNORE   /**< Totally ignores the error. */
  };

  /** @brief Set the error behaviour for encoding/decoding functions
   *
   *  @param p_et error type
   *  @param p_eb error behaviour
   */
  static void set_error_behavior(error_type_t p_et, error_behavior_t p_eb);

  /** @brief Get the current error behaviour
   *
   *  @param p_et error type
   *  @return error behaviour for the supplied error type
   */
  static error_behavior_t get_error_behavior(error_type_t p_et);

  /** @brief Get the default error behaviour
   *
   *  @param p_et error type
   *  @return default error behaviour for the supplied error type
   */
  static error_behavior_t get_default_error_behavior(error_type_t p_et);

  /** @brief Get the last error code.
   *
   *  @return last_error_type
   */
  static error_type_t get_last_error_type() { return last_error_type; }

  /** @brief Get the error string corresponding to the last error.
   *
   *  @return error_str
   */
  static const char* get_error_str() { return error_str; }

  /** @brief Set a clean slate
   *
   *  Sets last_error_type to ET_NONE.
   *  Frees error_str.
   */
  static void clear_error();

  /* The stuff below this line is for internal use only */
  static void error(error_type_t p_et, char *msg);

private:
  /** Default error behaviours for all error types */
  static const error_behavior_t default_error_behavior[ET_ALL];

  /** Current error behaviours for all error types */
  static error_behavior_t error_behavior[ET_ALL];

  /** Last error value */
  static error_type_t last_error_type;

  /** Error string for the last error */
  static char *error_str;
};

/** @brief Error context
 *
 *  Maintains a list of linked TTCN_EncDec_ErrorContext objects
 *
 */
class TTCN_EncDec_ErrorContext {
private:
  /** Variables for the linked list */
  static TTCN_EncDec_ErrorContext *head, *tail;

  /** Pointers for the linked list */
  TTCN_EncDec_ErrorContext *prev, *next;

  /** */
  char *msg;
  /// Copy constructor disabled
  TTCN_EncDec_ErrorContext(const TTCN_EncDec_ErrorContext&);
  /// Assignment disabled
  TTCN_EncDec_ErrorContext& operator=(const TTCN_EncDec_ErrorContext&);
public:
  /** @brief Default constructor.
   *
   *  Adds this object of the end of the list
   *
   *  @post msg == 0
   *  @post tail == this
   *
   */
  TTCN_EncDec_ErrorContext();

  /** @brief Constructor.
   *
   *  Adds this object of the end of the list
   *  and sets msg.
   *
   *  @post msg != 0
   *  @post tail == this
   *
   *  @param fmt printf-like format string
   *  @param ... printf-like parameters
   */
  TTCN_EncDec_ErrorContext(const char *fmt, ...)
    __attribute__ ((__format__ (__printf__, 2, 3))); // 'this' is at index 1

  /** @brief Destructor.
   *
   *  Removes this object from the end of the list
   *
   *  @pre tail == this
   *
   */
  ~TTCN_EncDec_ErrorContext();

  /** @brief Replaces msg with a new string
   *
   *  Calls Free, then mprintf_va_list
   *
   *  @param fmt printf-like format string
   *  @param ... printf-like parameters
   */
  void set_msg(const char *fmt, ...)
    __attribute__ ((__format__ (__printf__, 2, 3))); // 'this' is at index 1

  /** @brief Collects error strings from all active error contexts and logs an error
   *
   * If the error behaviour for \p p_et is not TTCN_EncDec::EB_IGNORE, then
   * walks the list of TTCN_EncDec_ErrorContext and concatenates the error messages,
   * then calls TTCN_EncDec::error
   *
   * @param p_et a TTCN_EncDec::error_type_t error type
   * @param fmt printf-like format string
   * @param ... printf-like parameters
   *
   */
  static void error(TTCN_EncDec::error_type_t p_et, const char *fmt, ...)
    __attribute__ ((__format__ (__printf__, 2, 3)));
  static void error_internal(const char *fmt, ...)
    __attribute__ ((__format__ (__printf__, 1, 2), __noreturn__));
  static void warning(const char *fmt, ...)
    __attribute__ ((__format__ (__printf__, 1, 2)));
};

/**
  * Buffer used by the different encoders/decoders.
  */
class TTCN_Buffer {
private:
  struct buffer_struct; // no user serviceable parts
  buffer_struct *buf_ptr; ///< pointer to internal data
  size_t buf_size;///< The number of bytes of memory allocated
  size_t buf_len; ///< The number of bytes of memory used (buf_len <= buf_size)
  size_t buf_pos; ///< Read offset in bytes in the buffer (buf_pos <= buf_len)
  size_t bit_pos; ///< Read offset of the current bit inside the byte at buf_pos
  size_t last_bit_pos;
  size_t last_bit_bitpos;
  size_t start_of_ext_bit;
  boolean last_bit; ///< the state of the "last bit" (only after get_b was called)
  boolean current_bitorder; ///< \c true for TOP_BIT_LEFT
  boolean ext_bit_reverse; ///< \c true if 0 signals the end
  unsigned int ext_level;

  /** Resets all fields except the size and length indicators. */
  void reset_buffer();
  /** Deallocates the memory area that is associated with the buffer. */
  void release_memory();
  /** Returns the smallest preferred size of the allocated memory area
   * that is at least \a target_size. */
  static size_t get_memory_size(size_t target_size);
  /** Ensures that buffer has its own writable memory area. */
  void copy_memory();
  /** Ensures that there are at least \a target_size writable bytes in the
   * memory area after \a buf_len. */
  void increase_size(size_t size_incr);

public:
  /** Creates an empty buffer. */
  TTCN_Buffer();
  /** Copy constructor. */
  TTCN_Buffer(const TTCN_Buffer& p_buf);
  /** Initializes the buffer with the contents of \a p_os.
   * @pre The argument must be bound. */
  TTCN_Buffer(const OCTETSTRING& p_os);
  /** Initializes the buffer with the contents of \a p_cs.
   * @pre The argument must be bound. */
  TTCN_Buffer(const CHARSTRING& p_cs);
  /** Destructor. */
  ~TTCN_Buffer() { release_memory(); }
  /** Copies the contents of \a p_buf into \a this.
   * The read pointers and other attributes are reset. */
  TTCN_Buffer& operator=(const TTCN_Buffer& p_buf);
  /** Copies the contents of \a p_os into \a this.
   * The read pointers and other attributes are reset. */
  TTCN_Buffer& operator=(const OCTETSTRING& p_os);
  /** Copies the contents of \a p_cs into \a this.
   * The read pointers and other attributes are reset. */
  TTCN_Buffer& operator=(const CHARSTRING& p_cs);
  /** Erases the content of the buffer. */
  void clear();
  /** Returns how many bytes are in the buffer. */
  inline size_t get_len() const { return buf_len; }
  /** Returns a pointer to the beginning of data. */
  const unsigned char* get_data() const;
  /** Returns how many bytes are in the buffer to read. */
  inline size_t get_read_len() const { return buf_len - buf_pos; }
  /** Returns a pointer which points to read position of data. */
  const unsigned char* get_read_data() const;
  /** Sets the read position to the beginning of the buffer. */
  inline void rewind() { buf_pos = 0; bit_pos = 0; }
  /** Returns the (reading) position of the buffer. */
  inline size_t get_pos() const { return buf_pos; }
  /** Sets the (reading) position to \a pos, or to the end of buffer,
    * if pos > len. */
  void set_pos(size_t pos);
  /** Increases the (reading) position by \a delta, or sets it to the
    * end of buffer, if get_pos() + delta > len. */
  void increase_pos(size_t delta);
  /** You can write up to \a end_len chars beginning from \a end_ptr;
    * after writing, you have to call also increase_length()! Useful
    * if you want to use memcpy. @see increase_length(). \param
    * end_ptr out. \param end_len inout. */
  void get_end(unsigned char*& end_ptr, size_t& end_len);
  /** How many chars have you written after a get_end(), beginning
    * from end_ptr. @see get_end() */
  void increase_length(size_t size_incr);
  /** Appends single character \a c to the buffer. */
  void put_c(unsigned char c);
  /** Appends \a len bytes starting from address \a s to the buffer. */
  void put_s(size_t len, const unsigned char *s);
  /** Appends the contents of octetstring \a p_os to the buffer. */
  void put_string(const OCTETSTRING& p_os);
  /** Same as \a put_string(). Provided only for backward compatibility. */
  inline void put_os(const OCTETSTRING& p_os) { put_string(p_os); }
  /** Appends the contents of charstring \a p_cs to the buffer. */
  void put_string(const CHARSTRING& p_cs);
  /** Same as \a put_string(). Provided only for backward compatibility. */
  inline void put_cs(const CHARSTRING& p_cs) { put_string(p_cs); }
  /** Appends the content of \a p_buf to the buffer */
  void put_buf(const TTCN_Buffer& p_buf);
  /** Stores the current contents of the buffer to variable \a p_os. */
  void get_string(OCTETSTRING& p_os);
  /** Stores the current contents of the buffer to variable \a p_cs. */
  void get_string(CHARSTRING& p_cs);
  /** Stores the current contents of the buffer to variable \a p_cs. */
  void get_string(UNIVERSAL_CHARSTRING& p_cs);
  /** Cuts the bytes between the beginning of the buffer and the read
    * position. After that the read position will point to the beginning
    * of the updated buffer. */
  void cut();
  /** Cuts the bytes between the read position and the end of the buffer.
    * The read position remains unchanged (i.e. it will point to the end
    * of the truncated buffer. */
  void cut_end();
  /** Returns whether the buffer (beginning from the read position)
    * contains a complete TLV. */
  boolean contains_complete_TLV();

  /* The members below this line are for internal use only. */
  /* ------------------------------------------------------ */

  void log() const;

  /** Puts a bit string in the buffer. Use only this function if you
    * use the buffer as bit buffer.
    *
    * @param len number of _bits_ to write
    * @param s pointer to the data (bytes)
    * @param coding_par
    * @param align alignment length (in ???)
    */
  void put_b(size_t len, const unsigned char *s,
             const RAW_coding_par& coding_par, int align);
  /** Reads a bit string from the buffer. Use only this function if you
    * use the buffer as bit buffer. */
  void get_b(size_t len, unsigned char *s, const RAW_coding_par& coding_par,
             raw_order_t top_bit_order);
  /** Puts @p len number of zeros in the buffer. */
  void put_zero(size_t len, raw_order_t fieldorder);
  /** Returns a pointer which points to read position of data and the
    * starting position of the bitstring within first the octet. */
  const unsigned char* get_read_data(size_t& bitpos) const;
  /** Sets the (reading) position to \a pos and the bit position to \a
    * bit_pos, or to the end of buffer, if pos > len. */
  void set_pos(size_t pos, size_t bitpos);
  /** Sets the (reading) position to \a pos
    * or to the end of buffer, if pos > len. */
  void set_pos_bit(size_t new_bit_pos);
  /** Returns the (reading) position of the buffer in bits. */
  inline size_t get_pos_bit() const { return buf_pos * 8 + bit_pos; }
  /** Increases the (reading) position by \a delta bits, or sets it to
    * the end of buffer, if get_pos() + delta > len. */
  void increase_pos_bit(size_t delta);
  /** Increases the (reading) position to a multiple of \p padding.
   *  @return the number of bits used up. */
  int increase_pos_padd(int padding);
  /** Returns the number of bits remaining in the buffer */
  size_t unread_len_bit();
  /** Mark the start of extension bit processing during encoding. */
  void start_ext_bit(boolean p_reverse);
  /** Apply the extension bit to the encoded bytes. */
  void stop_ext_bit();
  inline boolean get_order() const { return current_bitorder; }
  inline void set_order(boolean new_order) { current_bitorder = new_order; }
  void put_pad(size_t len, const unsigned char *s, int pat_len,
               raw_order_t fieldorder);
  void set_last_bit(boolean p_last_bit);
  inline boolean get_last_bit() const { return last_bit; }
  
private:
  static unsigned char get_byte_rev(const unsigned char* data, size_t len,
                             size_t index);
  unsigned char get_byte_align(size_t len, raw_order_t fieldorder,
                               raw_order_t req_align ,size_t index);
};

#endif
