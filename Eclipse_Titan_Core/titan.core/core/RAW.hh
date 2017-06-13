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
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *
 ******************************************************************************/
#ifndef _RAW_HH
#define _RAW_HH

#include "Types.h"
#include "Encdec.hh"
#include "CharCoding.hh"

struct bignum_st;
typedef bignum_st BIGNUM;

#define RAW_INT_ENC_LENGTH 4
#define REVERSE_BITS(b) (BitReverseTable[(b)&0xFF])

#define RAW_INTX -1

/**
 * \defgroup RAW RAW-related stuff.
 *
 * The RAW encoder/decoder can be used to handle protocols where the position
 * of information elements must be specified with bit-level precision.
 *
 * @{
 *
 * The RAW encoder is a two-pass encoder. In the first pass, information about
 * all the information elements is collected in a RAW_enc_tree object.
 * This information is used to write the actual encoding into the buffer.
 *
 * This two-pass mechanism is needed because the contents of earlier fields
 * can depend on fields which have not been encoded yet (e.g. length or
 * CROSSTAG selector fields).
 */

extern const unsigned char BitReverseTable[256];
extern const unsigned char BitMaskTable[9];
class RAW_enc_tree;
struct TTCN_Typedescriptor_t;
struct TTCN_TEXTdescriptor_t;
int min_bits(int a);
int min_bits(BIGNUM *a);
enum ext_bit_t{
  EXT_BIT_NO,      /**< No extension bit */
  EXT_BIT_YES,     /**< Extension bit used 0: more octets, 1: no more octets */
  EXT_BIT_REVERSE  /**< Extension bit used 1: more octets, 0: no more octets */
};
enum top_bit_order_t{
  TOP_BIT_INHERITED,
  TOP_BIT_LEFT,
  TOP_BIT_RIGHT
};
enum raw_sign_t{
  SG_NO,         /**< no sign, value coded as positive number */
  SG_2COMPL,     /**< the value coded as 2s complement */
  SG_SG_BIT      /**< the MSB used to encode the sign of the value */
};
/** position indicator of the encoding tree
 */
struct RAW_enc_tr_pos{
  int level;
  int *pos;
};

struct RAW_enc_pointer{
  RAW_enc_tr_pos target;
  int ptr_offset;
  int unit;
  int ptr_base;
};

struct RAW_enc_lengthto{
  int num_of_fields;
  RAW_enc_tr_pos* fields;
  int unit;
};

struct RAW_coding_par{
  raw_order_t bitorder;
  raw_order_t byteorder;
  raw_order_t hexorder;
  raw_order_t fieldorder;
};

/** RAW attributes for runtime */
struct TTCN_RAWdescriptor_t{
  int fieldlength; /**< length of field in \a unit s */
  raw_sign_t comp; /**< the method used for storing negative numbers */
  raw_order_t byteorder;
  raw_order_t endianness;
  raw_order_t bitorderinfield;
  raw_order_t bitorderinoctet;
  ext_bit_t extension_bit; /**< MSB mangling */
  raw_order_t hexorder;
  raw_order_t fieldorder;
  top_bit_order_t top_bit_order;
  int padding;
  int prepadding;
  int ptroffset;
  int unit; /**< number of bits per unit */
  int padding_pattern_length;
  const unsigned char* padding_pattern;
  int length_restrition;
  CharCoding::CharCodingType stringformat;
};

enum calc_type {
  CALC_NO, /**< not a calculated field */
  CALC_LENGTH, /**< calculated field for LENGTHTO */
  CALC_POINTER /**< calculated field for POINTERTO */
};

/** RAW encoding tree class.
 *  Collects the result of RAW encoding each component, recursively.
 *   */
class RAW_enc_tree{
public:
  /** indicates that the node is leaf (contains actual data) or not
   *  (contains pointers to other nodes) */
  boolean isleaf;
  boolean must_free;  /**< data_ptr was allocated, must be freed */
  boolean data_ptr_used; /**< Whether data_ptr member is used, not data_array */
  boolean rec_of;
  RAW_enc_tree *parent;
  RAW_enc_tr_pos curr_pos;
  int length;  /**< Encoded length */
  /** @name Encoding parameters related to the filling of the buffer @{ */
  int padding;
  int prepadding;
  int startpos;
  int padlength;
  int prepadlength;
  int padding_pattern_length;
  const unsigned char* padding_pattern;
  int align; /**< alignment length */
  /** @} */
  int ext_bit_handling; /**< 1: start, 2: stop, 3: only this */
  ext_bit_t ext_bit;
  top_bit_order_t top_bit_order;
  const TTCN_Typedescriptor_t *coding_descr;
  calc_type calc; /**< is it a calculated field or not */
  RAW_coding_par coding_par;
  union{ /** depends on calc */
    RAW_enc_lengthto lengthto; /**< calc is CALC_LENGTH */
    RAW_enc_pointer  pointerto; /**< calc is CALC_POINTER */
  } calcof;
  union{ /** depends on isleaf */
    struct{
      int num_of_nodes;
      RAW_enc_tree **nodes;
    } node; /**< isleaf is FALSE */
    /** depends on data_ptr_used */
    union{
      unsigned char *data_ptr;  /**< data_ptr_used==true */
      unsigned char data_array[RAW_INT_ENC_LENGTH];  /**< false */
    } leaf; /**< isleaf is TRUE */
  } body;
  RAW_enc_tree(boolean is_leaf, RAW_enc_tree *par, RAW_enc_tr_pos *par_pos,
               int my_pos,const TTCN_RAWdescriptor_t *raw_attr);
  ~RAW_enc_tree();

  /** Transfers the encoded information to the buffer.
   * @param buf buffer to receive the encoded data.
   * Calls calc_padding, calc_fields and fill_buf.
   */
  void put_to_buf(TTCN_Buffer &buf);
  RAW_enc_tree* get_node(RAW_enc_tr_pos&);
private:
  void fill_buf(TTCN_Buffer &);
  int calc_padding(int);
  void calc_fields();
  /// Copy constructor disabled
  RAW_enc_tree(const RAW_enc_tree&);
  /// Assignment disabled
  RAW_enc_tree& operator=(const RAW_enc_tree&);
};
struct TTCN_Typedescriptor_t;
int RAW_encode_enum_type(const TTCN_Typedescriptor_t&, RAW_enc_tree&,
                            int, int);
int RAW_decode_enum_type(const TTCN_Typedescriptor_t&,
                            TTCN_Buffer&, int, raw_order_t, int&,
                            int, boolean no_err=FALSE);

/// Allocate \p nodes_num pointers
RAW_enc_tree** init_nodes_of_enc_tree(int nodes_num);
/// Allocate \p num structures
RAW_enc_tr_pos* init_lengthto_fields_list(int num);
int* init_new_tree_pos(RAW_enc_tr_pos &old_pos,int new_levels, int* new_pos);
void free_tree_pos(int* ptr);
int min_of_ints(int num_of_int, ...);

extern const TTCN_RAWdescriptor_t INTEGER_raw_;
extern const TTCN_RAWdescriptor_t BOOLEAN_raw_;
extern const TTCN_RAWdescriptor_t BITSTRING_raw_;
extern const TTCN_RAWdescriptor_t OCTETSTRING_raw_;
extern const TTCN_RAWdescriptor_t CHARSTRING_raw_;
extern const TTCN_RAWdescriptor_t HEXSTRING_raw_;
extern const TTCN_RAWdescriptor_t FLOAT_raw_;
extern const TTCN_RAWdescriptor_t UNIVERSAL_CHARSTRING_raw_;

/** @} end of RAW group */

inline int bigger(int l, int r) {
  return l < r ? r: l;
}

inline int smaller(int l, int r) {
  return l < r ? l : r;
}


#endif // _RAW_HH
